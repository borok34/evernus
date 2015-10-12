/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ctime>

#include <QNetworkInterface>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QClipboard>
#include <QStatusBar>
#include <QTabWidget>
#include <QSettings>
#include <QMenuBar>
#include <QTabBar>
#include <QLabel>
#include <QDebug>

#ifdef Q_OS_WIN
#   if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
#       include <windows.h>
#   endif
#   include <sys/utime.h>
#   include <QWinTaskbarButton>
#else
#   include <utime.h>
#endif

#include "WalletTransactionsWidget.h"
#include "CharacterManagerDialog.h"
#include "MarketAnalysisWidget.h"
#include "WalletJournalWidget.h"
#include "CharacterRepository.h"
#include "MarketBrowserWidget.h"
#include "RepositoryProvider.h"
#include "ActiveTasksDialog.h"
#include "PreferencesDialog.h"
#include "MarketOrderWidget.h"
#include "ItemHistoryWidget.h"
#include "MarginToolDialog.h"
#include "StatisticsWidget.h"
#include "CharacterWidget.h"
#include "ContractWidget.h"
#include "ItemCostWidget.h"
#include "ImportSettings.h"
#include "KeyRepository.h"
#include "MenuBarWidget.h"
#include "PriceSettings.h"
#include "SyncSettings.h"
#include "HttpSettings.h"
#include "AssetsWidget.h"
#include "LMeveWidget.h"
#include "AboutDialog.h"
#include "IGBSettings.h"
#include "SyncDialog.h"
#include "UISettings.h"
#include "Updater.h"

#include "MainWindow.h"

namespace Evernus
{
    const QString MainWindow::settingsMaximizedKey = "mainWindow/maximized";
    const QString MainWindow::settingsPosKey = "mainWindow/pos";
    const QString MainWindow::settingsSizeKey = "mainWindow/size";

    MainWindow::MainWindow(const RepositoryProvider &repositoryProvider,
                           MarketOrderProvider &orderProvider,
                           MarketOrderProvider &corpOrderProvider,
                           const AssetProvider &assetProvider,
                           const ContractProvider &contractProvider,
                           const ContractProvider &corpContractProvider,
                           EveDataProvider &eveDataProvider,
                           const CacheTimerProvider &cacheTimerProvider,
                           ItemCostProvider &itemCostProvider,
                           const LMeveDataProvider &lMeveDataProvider,
                           TaskManager &taskManager,
                           QByteArray crestClientId,
                           QByteArray crestClientSecret,
                           QWidget *parent,
                           Qt::WindowFlags flags)
        : QMainWindow{parent, flags}
        , mRepositoryProvider{repositoryProvider}
        , mItemCostProvider{itemCostProvider}
        , mEveDataProvider{eveDataProvider}
        , mTrayIcon{new QSystemTrayIcon{QIcon{":/images/main-icon.png"}, this}}
    {
        readSettings();
        createMenu();
        createMainView(orderProvider,
                       corpOrderProvider,
                       assetProvider,
                       contractProvider,
                       corpContractProvider,
                       lMeveDataProvider,
                       cacheTimerProvider,
                       taskManager,
                       std::move(crestClientId),
                       std::move(crestClientSecret));
        createStatusBar();

        connect(mTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::activateTrayIcon);
        connect(&mAutoImportTimer, &QTimer::timeout, this, &MainWindow::refreshAll);

        setWindowIcon(QIcon{":/images/main-icon.png"});

        if (!mRepositoryProvider.getCharacterRepository().hasCharacters())
            QMetaObject::invokeMethod(this, "showCharacterManagement", Qt::QueuedConnection);

        setUpAutoImportTimer();
    }

    void MainWindow::showAsSaved()
    {
        if (mShowMaximized)
            showMaximized();
        else
            show();
    }

    void MainWindow::showCharacterManagement()
    {
        if (mCharacterManagerDialog == nullptr)
        {
            mCharacterManagerDialog = new CharacterManagerDialog{mRepositoryProvider.getCharacterRepository(),
                                                                 mRepositoryProvider.getKeyRepository(),
                                                                 mRepositoryProvider.getCorpKeyRepository(),
                                                                 this};
            connect(this, &MainWindow::charactersChanged, mCharacterManagerDialog, &CharacterManagerDialog::updateCharacters);
            connect(mCharacterManagerDialog, &CharacterManagerDialog::refreshCharacters, this, &MainWindow::refreshCharacters);
            connect(mCharacterManagerDialog, &CharacterManagerDialog::charactersChanged,
                    mMenuWidget, &MenuBarWidget::refreshCharacters);
            connect(mCharacterManagerDialog, &CharacterManagerDialog::charactersChanged,
                    this, &MainWindow::updateCharacters);
        }

        mCharacterManagerDialog->exec();
    }

    void MainWindow::showPreferences()
    {
        PreferencesDialog dlg{this};
        connect(&dlg, &PreferencesDialog::clearCorpWalletData, this, &MainWindow::clearCorpWalletData);

        dlg.exec();

        setUpAutoImportTimer();
        mFPCController.handleNewPreferences();

        emit preferencesChanged();
    }

    void MainWindow::showMarginTool()
    {
        QSettings settings;
        if (settings.value(UISettings::minimizeByMarginToolKey, UISettings::minimizeByMarginToolDefault).toBool())
            showMinimized();

        if (mMarginToolDialog.isNull())
        {
            mMarginToolDialog = new MarginToolDialog{mRepositoryProvider.getCharacterRepository(),
                                                     mItemCostProvider,
                                                     mEveDataProvider};
            mMarginToolDialog->setCharacter(mCurrentCharacterId);
            connect(mMenuWidget, &MenuBarWidget::currentCharacterChanged, mMarginToolDialog, &MarginToolDialog::setCharacter);
            connect(mMarginToolDialog, &MarginToolDialog::hidden, this, &MainWindow::showNormal);
            connect(mMarginToolDialog, &MarginToolDialog::quit, this, &MainWindow::close);
            connect(this, &MainWindow::preferencesChanged, mMarginToolDialog, &MarginToolDialog::handleNewPreferences);
        }

        mMarginToolDialog->showNormal();
        mMarginToolDialog->activateWindow();
    }

    void MainWindow::showAbout()
    {
        AboutDialog dlg{this};
        dlg.exec();
    }

    void MainWindow::openHelp()
    {
        QDesktopServices::openUrl(QUrl{"http://evernus.com/help"});
    }

    void MainWindow::checkForUpdates()
    {
        Updater::getInstance().checkForUpdates(false);
    }

    void MainWindow::showColumnHelp()
    {
        QMessageBox::information(this, tr("Evernus"), tr(
            "You can show/hide table columns via right-click menu. Columns can also be moved around via dragging."));
    }

    void MainWindow::addNewTaskInfo(uint taskId, const QString &description)
    {
        if (mActiveTasksDialog == nullptr)
        {
#ifdef Q_OS_WIN
            if (mTaskbarButton == nullptr)
            {
                mTaskbarButton = new QWinTaskbarButton{this};
                mTaskbarButton->setWindow(windowHandle());
            }

            mActiveTasksDialog = new ActiveTasksDialog{*mTaskbarButton, this};
#else
            mActiveTasksDialog = new ActiveTasksDialog{this};
#endif
            connect(this, &MainWindow::newTaskInfoAdded, mActiveTasksDialog, &ActiveTasksDialog::addNewTaskInfo);
            connect(this, &MainWindow::newSubTaskInfoAdded, mActiveTasksDialog, &ActiveTasksDialog::addNewSubTaskInfo);
            connect(this, &MainWindow::taskInfoChanged, mActiveTasksDialog, &ActiveTasksDialog::setTaskInfo);
            connect(this, &MainWindow::taskEnded, mActiveTasksDialog, &ActiveTasksDialog::endTask);
            mActiveTasksDialog->setAttribute(Qt::WA_ShowWithoutActivating);
            mActiveTasksDialog->setModal(true);
            mActiveTasksDialog->show();
        }
        else
        {
            mActiveTasksDialog->show();
        }

        emit newTaskInfoAdded(taskId, description);
    }

    void MainWindow::updateIskData()
    {
        if (mCurrentCharacterId != Character::invalidId)
        {
            const auto character = mRepositoryProvider.getCharacterRepository().find(mCurrentCharacterId);
            mStatusWalletLabel->setText(tr("Wallet: <strong>%1</strong>")
                .arg(character->getISKPresentation()));
        }
        else
        {
            mStatusWalletLabel->setText(QString{});
        }
    }

    void MainWindow::updateCharacters()
    {
        refreshCharactersMenu();
        emit charactersChanged();
    }

    void MainWindow::setCharacter(Character::IdType id)
    {
        mCurrentCharacterId = id;
        updateIskData();
        updateCurrentTab(mMainTabs->currentIndex());
    }

    void MainWindow::refreshAssets()
    {
        QSettings settings;
        if (settings.value(ImportSettings::importAllCharactersKey, ImportSettings::importAllCharactersDefault).toBool())
        {
            enumerateEnabledCharacters([this](auto id) {
                emit importAssets(id);
            });
        }
        else if (mCurrentCharacterId != Character::invalidId)
        {
            emit importAssets(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshContracts()
    {
        QSettings settings;
        if (settings.value(ImportSettings::importAllCharactersKey, ImportSettings::importAllCharactersDefault).toBool())
        {
            const auto corp = settings.value(ImportSettings::updateCorpDataKey).toBool();
            enumerateEnabledCharacters([corp, this](auto id) {
                if (corp)
                    emit importCorpContracts(id);

                emit importContracts(id);
            });
        }
        else if (mCurrentCharacterId != Character::invalidId)
        {
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpContracts(mCurrentCharacterId);

            emit importContracts(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshWalletJournal()
    {
        QSettings settings;
        if (settings.value(ImportSettings::importAllCharactersKey, ImportSettings::importAllCharactersDefault).toBool())
        {
            const auto corp = settings.value(ImportSettings::updateCorpDataKey).toBool();
            enumerateEnabledCharacters([corp, this](auto id) {
                if (corp)
                    emit importCorpWalletJournal(id);

                emit importWalletJournal(id);
            });
        }
        else if (mCurrentCharacterId != Character::invalidId)
        {
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpWalletJournal(mCurrentCharacterId);

            emit importWalletJournal(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshWalletTransactions()
    {
        QSettings settings;
        if (settings.value(ImportSettings::importAllCharactersKey, ImportSettings::importAllCharactersDefault).toBool())
        {
            const auto corp = settings.value(ImportSettings::updateCorpDataKey).toBool();
            enumerateEnabledCharacters([corp, this](auto id) {
                if (corp)
                    emit importCorpWalletTransactions(id);

                emit importWalletTransactions(id);
            });
        }
        else if (mCurrentCharacterId != Character::invalidId)
        {
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpWalletTransactions(mCurrentCharacterId);

            emit importWalletTransactions(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshMarketOrdersFromAPI()
    {
        QSettings settings;
        if (settings.value(ImportSettings::importAllCharactersKey, ImportSettings::importAllCharactersDefault).toBool())
        {
            const auto corp = settings.value(ImportSettings::updateCorpDataKey).toBool();
            enumerateEnabledCharacters([corp, this](auto id) {
                if (corp)
                    emit importCorpMarketOrdersFromAPI(id);

                emit importMarketOrdersFromAPI(id);
            });
        }
        else if (mCurrentCharacterId != Character::invalidId)
        {
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpMarketOrdersFromAPI(mCurrentCharacterId);

            emit importMarketOrdersFromAPI(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshMarketOrdersFromLogs()
    {
        QSettings settings;
        if (settings.value(ImportSettings::importAllCharactersKey, ImportSettings::importAllCharactersDefault).toBool())
        {
            const auto corp = settings.value(ImportSettings::updateCorpDataKey).toBool();
            enumerateEnabledCharacters([corp, this](auto id) {
                if (corp)
                    emit importCorpMarketOrdersFromLogs(id);

                emit importMarketOrdersFromLogs(id);
            });
        }
        else if (mCurrentCharacterId != Character::invalidId)
        {
            if (settings.value(ImportSettings::updateCorpDataKey).toBool())
                emit importCorpMarketOrdersFromLogs(mCurrentCharacterId);

            emit importMarketOrdersFromLogs(mCurrentCharacterId);
        }
    }

    void MainWindow::refreshAll()
    {
        emit refreshCharacters();
        emit refreshConquerableStations();

        refreshWalletJournal();

        QSettings settings;

        const auto marketOrderSource = static_cast<ImportSettings::MarketOrderImportSource>(
            settings.value(ImportSettings::marketOrderImportSourceKey, static_cast<int>(ImportSettings::marketOrderImportSourceDefault)).toInt());
        if (marketOrderSource == ImportSettings::MarketOrderImportSource::Logs)
            refreshMarketOrdersFromLogs();
        else
            refreshMarketOrdersFromAPI();

        if (!settings.value(PriceSettings::autoAddCustomItemCostKey, PriceSettings::autoAddCustomItemCostDefault).toBool())
            refreshWalletTransactions();

        if (settings.value(ImportSettings::importAssetsKey, ImportSettings::importAssetsDefault).toBool())
            refreshAssets();
        if (settings.value(ImportSettings::importContractsKey, ImportSettings::importContractsDefault).toBool())
            refreshContracts();
    }

    void MainWindow::updateCurrentTab(int index)
    {
        auto widget = mMainTabs->widget(index);
        if (widget != nullptr)
        {
            if (mCurrentCharacterId != mTabCharacterIds[index])
            {
                mTabCharacterIds[index] = mCurrentCharacterId;
                QMetaObject::invokeMethod(mTabWidgets[index], "setCharacter", Qt::AutoConnection, Q_ARG(Character::IdType, mCurrentCharacterId));
            }
#ifdef Q_OS_MAC
            statusBar()->repaint();
#endif
        }
    }

    void MainWindow::activateTrayIcon(QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        {
            mTrayIcon->hide();
            showMaximized();
            activateWindow();
        }
    }

    void MainWindow::copyIGBLink()
    {
        QSettings settings;
        QApplication::clipboard()->setText(QString{"http://localhost:%1"}
            .arg(settings.value(IGBSettings::portKey, IGBSettings::portDefault).value<quint16>()));

        QMessageBox::information(this, tr("Evernus"), tr("IGB link was copied to the clipboard."));
    }

    void MainWindow::copyHTTPLink()
    {
        const auto addresses = QNetworkInterface::allAddresses();
        QString curAddress = "localhost";

        for (const auto &address : addresses)
        {
            if (!address.isLoopback() && address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                curAddress = address.toString();
                break;
            }
        }

        QSettings settings;
        QApplication::clipboard()->setText(QString{"http://%1:%2"}
            .arg(curAddress)
            .arg(settings.value(HttpSettings::portKey, HttpSettings::portDefault).value<quint16>()));

        QMessageBox::information(this, tr("Evernus"), tr("HTTP link was copied to the clipboard."));
    }

    void MainWindow::showMarketBrowser()
    {
        mMainTabs->setCurrentIndex(mMarketBrowserTab);
    }

    void MainWindow::performSync()
    {
#ifdef EVERNUS_DROPBOX_ENABLED
        SyncDialog syncDlg{SyncDialog::Mode::Upload};
        syncDlg.exec();

        utimbuf buf;
        time(&buf.actime);
        buf.modtime = buf.actime;

        utime(mRepositoryProvider.getKeyRepository().getDatabase().databaseName().toUtf8().constData(), &buf);
#endif
    }

    void MainWindow::changeEvent(QEvent *event)
    {
        QSettings settings;
        if ((event->type() == QEvent::WindowStateChange) &&
            settings.value(UISettings::minimizeToTrayKey, UISettings::minimizeToTrayDefault).toBool())
        {
            if (isMinimized())
            {
                mTrayIcon->show();
                QMetaObject::invokeMethod(this, "hide", Qt::QueuedConnection);
            }
            else
            {
                mTrayIcon->hide();
            }
        }
    }

    void MainWindow::closeEvent(QCloseEvent *event)
    {
        if (!mMarginToolDialog.isNull())
            mMarginToolDialog->close();

        writeSettings();

        QSettings settings;
        if (settings.value(SyncSettings::enabledOnStartupKey, SyncSettings::enabledOnStartupDefault).toBool() &&
            settings.value(SyncSettings::enabledOnShutdownKey, SyncSettings::enabledOnShutdownDefault).toBool() &&
            SyncDialog::performedSync())
        {
            performSync();
        }

        event->accept();
    }

    void MainWindow::readSettings()
    {
        QSettings settings;

        const auto pos = settings.value(settingsPosKey).toPoint();
        const auto size = settings.value(settingsSizeKey, QSize{600, 400}).toSize();

        resize(size);

        const auto desktop = QApplication::desktop();
        const auto screenCount = desktop->screenCount();

        for (auto i = 0; i < screenCount; ++i)
        {
            if (desktop->screenGeometry(i).contains(pos))
            {
                move(pos);
                break;
            }
        }

        mShowMaximized = settings.value(settingsMaximizedKey, false).toBool();
    }

    void MainWindow::writeSettings()
    {
        QSettings settings;
        settings.setValue(settingsPosKey, pos());
        settings.setValue(settingsSizeKey, size());
        settings.setValue(settingsMaximizedKey, isMaximized());
    }

    void MainWindow::createMenu()
    {
        auto bar = menuBar();

        auto fileMenu = bar->addMenu(tr("&File"));
        mCharactersMenu = fileMenu->addMenu(tr("Select character"));
        fileMenu->addAction(QIcon{":/images/user.png"}, tr("&Manage characters..."), this, SLOT(showCharacterManagement()));
#ifdef Q_OS_OSX
        fileMenu->addAction(QIcon{":/images/wrench.png"}, tr("&Preferences..."), this, SLOT(showPreferences()), QKeySequence::Preferences)->setMenuRole(QAction::PreferencesRole);
#else
        fileMenu->addAction(QIcon{":/images/wrench.png"}, tr("&Preferences..."), this, SLOT(showPreferences()), Qt::CTRL + Qt::Key_O)->setMenuRole(QAction::PreferencesRole);
#endif
        fileMenu->addSeparator();
        fileMenu->addAction(tr("Import EVE Mentat order history..."), this, SIGNAL(importFromMentat()));
        fileMenu->addSeparator();
        fileMenu->addAction(tr("E&xit"), this, SLOT(close()), QKeySequence::Quit)->setMenuRole(QAction::QuitRole);

        auto toolsMenu = bar->addMenu(tr("&Tools"));
        toolsMenu->addAction(tr("Import conquerable stations"), this, SIGNAL(refreshConquerableStations()));
        toolsMenu->addAction(QIcon{":/images/report.png"}, tr("Ma&rgin tool..."), this, SLOT(showMarginTool()), Qt::CTRL + Qt::Key_M);
        toolsMenu->addSeparator();
        toolsMenu->addAction(tr("Copy HTTP link"), this, SLOT(copyHTTPLink()));
        toolsMenu->addAction(tr("Copy IGB link"), this, SLOT(copyIGBLink()));
#ifdef EVERNUS_DROPBOX_ENABLED
        toolsMenu->addSeparator();
        toolsMenu->addAction(QIcon{":/images/arrow_refresh.png"}, tr("Upload data to cloud..."), this, SLOT(performSync()));
#endif

        auto viewMenu = bar->addMenu(tr("&View"));
        mViewTabsMenu = viewMenu->addMenu(tr("Show/hide tabs"));
        viewMenu->addAction(tr("Show/hide table columns"), this, SLOT(showColumnHelp()));

        auto action = viewMenu->addAction(tr("Always on top"));
        auto toggleTopmost = [action, this] {
            const auto alwaysOnTop = action->isChecked();

            QSettings settings;
            settings.setValue(UISettings::mainWindowAlwaysOnTopKey, alwaysOnTop);

#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
            // https://bugreports.qt-project.org/browse/QTBUG-30359
            if (alwaysOnTop)
                SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            else
                SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
            auto flags = windowFlags();
            if (alwaysOnTop)
                flags |= Qt::WindowStaysOnTopHint;
            else
                flags &= ~Qt::WindowStaysOnTopHint;

            setWindowFlags(flags);
            show();
#endif
        };

        QSettings settings;

        action->setCheckable(true);
        action->setChecked(
            settings.value(UISettings::mainWindowAlwaysOnTopKey, UISettings::mainWindowAlwaysOnTopDefault).toBool());
        connect(action, &QAction::triggered, this, toggleTopmost);

        auto helpMenu = bar->addMenu(tr("&Help"));
        helpMenu->addAction(QIcon{":/images/help.png"}, tr("&Online help..."), this, SLOT(openHelp()));
        helpMenu->addAction(tr("Check for &updates"), this, SLOT(checkForUpdates()));
        helpMenu->addSeparator();
        helpMenu->addAction(tr("&About..."), this, SLOT(showAbout()))->setMenuRole(QAction::AboutRole);

        mMenuWidget = new MenuBarWidget{mRepositoryProvider.getCharacterRepository(), this};
        bar->setCornerWidget(mMenuWidget);
        connect(this, &MainWindow::charactersChanged, mMenuWidget, &MenuBarWidget::refreshCharacters);
        connect(mMenuWidget, &MenuBarWidget::currentCharacterChanged, this, &MainWindow::setCharacter);
        connect(mMenuWidget, &MenuBarWidget::importAll, this, &MainWindow::refreshAll);

        refreshCharactersMenu();
        toggleTopmost();
    }

    void MainWindow::createMainView(MarketOrderProvider &orderProvider,
                                    MarketOrderProvider &corpOrderProvider,
                                    const AssetProvider &assetProvider,
                                    const ContractProvider &contractProvider,
                                    const ContractProvider &corpContractProvider,
                                    const LMeveDataProvider &lMeveDataProvider,
                                    const CacheTimerProvider &cacheTimerProvider,
                                    TaskManager &taskManager,
                                    QByteArray crestClientId,
                                    QByteArray crestClientSecret)
    {
        mMainTabs = new QTabWidget{this};
        setCentralWidget(mMainTabs);
#ifdef Q_OS_OSX
        mMainTabs->tabBar()->setElideMode(Qt::ElideNone);
        mMainTabs->tabBar()->setUsesScrollButtons(true);
#endif
        mMainTabs->tabBar()->setStyleSheet("*::tab:disabled { width: 0; height: 0; margin: 0; padding: 0; border: none; }");
        connect(mMainTabs, &QTabWidget::currentChanged, this, &MainWindow::updateCurrentTab);

        auto charTab = new CharacterWidget{mRepositoryProvider.getCharacterRepository(),
                                           mRepositoryProvider.getMarketOrderRepository(),
                                           mRepositoryProvider.getCorpKeyRepository(),
                                           cacheTimerProvider,
                                           this};
        addTab(charTab, tr("Character"), TabType::Character);
        connect(charTab, &CharacterWidget::importFromAPI, this, &MainWindow::importCharacter);
        connect(charTab, &CharacterWidget::importAll, this, &MainWindow::refreshAll);
        connect(this, &MainWindow::charactersChanged, charTab, &CharacterWidget::updateData);
        connect(this, &MainWindow::assetsChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::walletJournalChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::walletTransactionsChanged, charTab, &CharacterWidget::updateTimerList);
        connect(this, &MainWindow::marketOrdersChanged, charTab, &CharacterWidget::updateMarketData);

        auto statsTab = new StatisticsWidget{mRepositoryProvider.getAssetValueSnapshotRepository(),
                                             mRepositoryProvider.getWalletSnapshotRepository(),
                                             mRepositoryProvider.getCorpWalletSnapshotRepository(),
                                             mRepositoryProvider.getMarketOrderValueSnapshotRepository(),
                                             mRepositoryProvider.getCorpMarketOrderValueSnapshotRepository(),
                                             mRepositoryProvider.getWalletJournalEntryRepository(),
                                             mRepositoryProvider.getWalletTransactionRepository(),
                                             mRepositoryProvider.getCorpWalletJournalEntryRepository(),
                                             mRepositoryProvider.getCorpWalletTransactionRepository(),
                                             mRepositoryProvider.getMarketOrderRepository(),
                                             mRepositoryProvider.getOrderScriptRepository(),
                                             mRepositoryProvider.getCharacterRepository(),
                                             mEveDataProvider,
                                             this};
        addTab(statsTab, tr("Statistics"), TabType::Other);
        connect(this, &MainWindow::charactersChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::externalOrdersChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::assetsChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletJournalChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletJournalChanged, statsTab, &StatisticsWidget::updateJournalData);
        connect(this, &MainWindow::corpWalletJournalChanged, statsTab, &StatisticsWidget::updateJournalData);
        connect(this, &MainWindow::corpWalletJournalChanged, statsTab, &StatisticsWidget::updateBalanceData);
        connect(this, &MainWindow::walletTransactionsChanged, statsTab, &StatisticsWidget::updateTransactionData);
        connect(this, &MainWindow::corpWalletTransactionsChanged, statsTab, &StatisticsWidget::updateTransactionData);
        connect(this, &MainWindow::preferencesChanged, statsTab, &StatisticsWidget::handleNewPreferences);

        auto assetsTab = new AssetsWidget{assetProvider,
                                          mEveDataProvider,
                                          cacheTimerProvider,
                                          mRepositoryProvider.getFilterTextRepository(),
                                          this};
        addTab(assetsTab, tr("Assets"), TabType::Other);
        connect(assetsTab, &AssetsWidget::importFromAPI, this, &MainWindow::importAssets);
        connect(assetsTab, &AssetsWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(assetsTab, &AssetsWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(assetsTab, &AssetsWidget::setDestinationInEve, this, &MainWindow::setDestinationInEve);
        connect(this, &MainWindow::conquerableStationsChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::assetsChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::externalOrdersChangedWithMarketOrders, assetsTab, &AssetsWidget::updateData);
        connect(this, &MainWindow::itemVolumeChanged, assetsTab, &AssetsWidget::updateData);

        auto orderTab = new MarketOrderWidget{orderProvider,
                                              corpOrderProvider,
                                              cacheTimerProvider,
                                              mEveDataProvider,
                                              mItemCostProvider,
                                              mRepositoryProvider.getWalletTransactionRepository(),
                                              mRepositoryProvider.getCharacterRepository(),
                                              mRepositoryProvider.getFilterTextRepository(),
                                              mRepositoryProvider.getExternalOrderRepository(),
                                              false,
                                              this};
        addTab(orderTab, tr("Character orders"), TabType::Character);
        connect(orderTab, &MarketOrderWidget::importFromAPI, this, &MainWindow::importMarketOrdersFromAPI);
        connect(orderTab, &MarketOrderWidget::importFromLogs, this, &MainWindow::importMarketOrdersFromLogs);
        connect(orderTab, &MarketOrderWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(orderTab, &MarketOrderWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(orderTab, &MarketOrderWidget::openMarginTool, this, &MainWindow::showMarginTool);
        connect(orderTab, &MarketOrderWidget::showExternalOrders, this, &MainWindow::showMarketBrowser);
        connect(orderTab, &MarketOrderWidget::showInEve, this, &MainWindow::showInEve);
        connect(orderTab, &MarketOrderWidget::fpcExecutorChanged, &mFPCController, &FPCController::changeExecutor);
        connect(this, &MainWindow::marketOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::corpMarketOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::conquerableStationsChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::itemCostsChanged, orderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::charactersChanged, orderTab, &MarketOrderWidget::updateCharacters);

        auto journalTab = new WalletJournalWidget{mRepositoryProvider.getWalletJournalEntryRepository(),
                                                  mRepositoryProvider.getCharacterRepository(),
                                                  mRepositoryProvider.getFilterTextRepository(),
                                                  cacheTimerProvider,
                                                  mEveDataProvider,
                                                  false,
                                                  this};
        addTab(journalTab, tr("Character journal"), TabType::Character);
        connect(journalTab, &WalletJournalWidget::importFromAPI, this, &MainWindow::importWalletJournal);
        connect(this, &MainWindow::walletJournalChanged, journalTab, &WalletJournalWidget::updateData);

        auto transactionsTab = new WalletTransactionsWidget{mRepositoryProvider.getWalletTransactionRepository(),
                                                            mRepositoryProvider.getCharacterRepository(),
                                                            mRepositoryProvider.getFilterTextRepository(),
                                                            cacheTimerProvider,
                                                            mEveDataProvider,
                                                            mItemCostProvider,
                                                            false,
                                                            this};
        addTab(transactionsTab, tr("Character transactions"), TabType::Character);
        connect(transactionsTab, &WalletTransactionsWidget::importFromAPI, this, &MainWindow::importWalletTransactions);
        connect(this, &MainWindow::walletTransactionsChanged, transactionsTab, &WalletTransactionsWidget::updateData);
        connect(this, &MainWindow::charactersChanged, transactionsTab, &WalletTransactionsWidget::updateCharacters);

        auto contractsTab = new ContractWidget{cacheTimerProvider,
                                               mEveDataProvider,
                                               contractProvider,
                                               mRepositoryProvider.getFilterTextRepository(),
                                               mRepositoryProvider.getCharacterRepository(),
                                               false,
                                               this};
        addTab(contractsTab, tr("Character contracts"), TabType::Character);
        connect(contractsTab, &ContractWidget::importFromAPI, this, &MainWindow::importContracts);
        connect(this, &MainWindow::contractsChanged, contractsTab, &ContractWidget::updateData);

        auto corpOrderTab = new MarketOrderWidget{corpOrderProvider,
                                                  corpOrderProvider,
                                                  cacheTimerProvider,
                                                  mEveDataProvider,
                                                  mItemCostProvider,
                                                  mRepositoryProvider.getCorpWalletTransactionRepository(),
                                                  mRepositoryProvider.getCharacterRepository(),
                                                  mRepositoryProvider.getFilterTextRepository(),
                                                  mRepositoryProvider.getExternalOrderRepository(),
                                                  true,
                                                  this};
        addTab(corpOrderTab, tr("Corporation orders"), TabType::Corp);
        connect(corpOrderTab, &MarketOrderWidget::importFromAPI, this, &MainWindow::importCorpMarketOrdersFromAPI);
        connect(corpOrderTab, &MarketOrderWidget::importFromLogs, this, &MainWindow::importCorpMarketOrdersFromLogs);
        connect(corpOrderTab, &MarketOrderWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(corpOrderTab, &MarketOrderWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(corpOrderTab, &MarketOrderWidget::openMarginTool, this, &MainWindow::showMarginTool);
        connect(corpOrderTab, &MarketOrderWidget::showExternalOrders, this, &MainWindow::showMarketBrowser);
        connect(corpOrderTab, &MarketOrderWidget::showInEve, this, &MainWindow::showInEve);
        connect(corpOrderTab, &MarketOrderWidget::fpcExecutorChanged, &mFPCController, &FPCController::changeExecutor);
        connect(this, &MainWindow::corpMarketOrdersChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::externalOrdersChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::conquerableStationsChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::itemCostsChanged, corpOrderTab, &MarketOrderWidget::updateData);
        connect(this, &MainWindow::charactersChanged, corpOrderTab, &MarketOrderWidget::updateCharacters);

        auto corpJournalTab = new WalletJournalWidget{mRepositoryProvider.getCorpWalletJournalEntryRepository(),
                                                      mRepositoryProvider.getCharacterRepository(),
                                                      mRepositoryProvider.getFilterTextRepository(),
                                                      cacheTimerProvider,
                                                      mEveDataProvider,
                                                      true,
                                                      this};
        addTab(corpJournalTab, tr("Corporation journal"), TabType::Corp);
        connect(corpJournalTab, &WalletJournalWidget::importFromAPI, this, &MainWindow::importCorpWalletJournal);
        connect(this, &MainWindow::corpWalletJournalChanged, corpJournalTab, &WalletJournalWidget::updateData);

        auto corpTransactionsTab = new WalletTransactionsWidget{mRepositoryProvider.getCorpWalletTransactionRepository(),
                                                                mRepositoryProvider.getCharacterRepository(),
                                                                mRepositoryProvider.getFilterTextRepository(),
                                                                cacheTimerProvider,
                                                                mEveDataProvider,
                                                                mItemCostProvider,
                                                                true,
                                                                this};
        addTab(corpTransactionsTab, tr("Corporation transactions"), TabType::Corp);
        connect(corpTransactionsTab, &WalletTransactionsWidget::importFromAPI, this, &MainWindow::importCorpWalletTransactions);
        connect(this, &MainWindow::corpWalletTransactionsChanged, corpTransactionsTab, &WalletTransactionsWidget::updateData);
        connect(this, &MainWindow::charactersChanged, corpTransactionsTab, &WalletTransactionsWidget::updateCharacters);

        auto corpContractsTab = new ContractWidget{cacheTimerProvider,
                                                   mEveDataProvider,
                                                   corpContractProvider,
                                                   mRepositoryProvider.getFilterTextRepository(),
                                                   mRepositoryProvider.getCharacterRepository(),
                                                   true,
                                                   this};
        addTab(corpContractsTab, tr("Corporation contracts"), TabType::Corp);
        connect(corpContractsTab, &ContractWidget::importFromAPI, this, &MainWindow::importCorpContracts);
        connect(this, &MainWindow::corpContractsChanged, corpContractsTab, &ContractWidget::updateData);

        auto itemHistoryTab = new ItemHistoryWidget{mRepositoryProvider.getWalletTransactionRepository(),
                                                    mRepositoryProvider.getCorpWalletTransactionRepository(),
                                                    mEveDataProvider,
                                                    this};
        connect(this, &MainWindow::walletTransactionsChanged, itemHistoryTab, &ItemHistoryWidget::updateData);
        connect(this, &MainWindow::corpWalletTransactionsChanged, itemHistoryTab, &ItemHistoryWidget::updateData);
        connect(this, &MainWindow::preferencesChanged, itemHistoryTab, &ItemHistoryWidget::handleNewPreferences);
        addTab(itemHistoryTab, tr("Item history"), TabType::Other);

        auto marketBrowserTab = new MarketBrowserWidget{mRepositoryProvider.getExternalOrderRepository(),
                                                        mRepositoryProvider.getMarketOrderRepository(),
                                                        mRepositoryProvider.getCorpMarketOrderRepository(),
                                                        mRepositoryProvider.getCharacterRepository(),
                                                        mRepositoryProvider.getFavoriteItemRepository(),
                                                        mRepositoryProvider.getLocationBookmarkRepository(),
                                                        orderProvider,
                                                        corpOrderProvider,
                                                        mEveDataProvider,
                                                        mItemCostProvider,
                                                        this};
        mMarketBrowserTab = addTab(marketBrowserTab, tr("Market browser"), TabType::Other);
        connect(marketBrowserTab, &MarketBrowserWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(marketBrowserTab, &MarketBrowserWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(marketBrowserTab, &MarketBrowserWidget::externalOrdersChanged, this, &MainWindow::externalOrdersChanged);
        connect(corpOrderTab, &MarketOrderWidget::showExternalOrders, marketBrowserTab, &MarketBrowserWidget::showOrdersForType);
        connect(orderTab, &MarketOrderWidget::showExternalOrders, marketBrowserTab, &MarketBrowserWidget::showOrdersForType);
        connect(this, &MainWindow::marketOrdersChanged, marketBrowserTab, &MarketBrowserWidget::fillOrderItemNames);
        connect(this, &MainWindow::corpMarketOrdersChanged, marketBrowserTab, &MarketBrowserWidget::fillOrderItemNames);
        connect(this, &MainWindow::externalOrdersChanged, marketBrowserTab, &MarketBrowserWidget::updateData);
        connect(this, &MainWindow::itemVolumeChanged, marketBrowserTab, &MarketBrowserWidget::updateData);

        auto itemCostTab = new ItemCostWidget{mItemCostProvider, mEveDataProvider, this};
        addTab(itemCostTab, tr("Item costs"), TabType::Other);
        connect(this, &MainWindow::itemCostsChanged, itemCostTab, &ItemCostWidget::updateData);

        auto lmEveTab = new LMeveWidget{cacheTimerProvider,
                                        mEveDataProvider,
                                        lMeveDataProvider,
                                        mItemCostProvider,
                                        mRepositoryProvider.getCharacterRepository(),
                                        this};
        addTab(lmEveTab, tr("LMeve"), TabType::Other);
        connect(lmEveTab, &LMeveWidget::openPreferences, this, &MainWindow::showPreferences);
        connect(lmEveTab, &LMeveWidget::syncLMeve, this, &MainWindow::syncLMeve);
        connect(lmEveTab, &LMeveWidget::importPricesFromWeb, this, &MainWindow::importExternalOrdersFromWeb);
        connect(lmEveTab, &LMeveWidget::importPricesFromFile, this, &MainWindow::importExternalOrdersFromFile);
        connect(this, &MainWindow::externalOrdersChanged, lmEveTab, &LMeveWidget::updateData);
        connect(this, &MainWindow::lMeveTasksChanged, lmEveTab, &LMeveWidget::updateData);

        auto marketAnalysisTab = new MarketAnalysisWidget{std::move(crestClientId),
                                                          std::move(crestClientSecret),
                                                          mEveDataProvider,
                                                          taskManager,
                                                          mRepositoryProvider.getMarketOrderRepository(),
                                                          mRepositoryProvider.getEveTypeRepository(),
                                                          mRepositoryProvider.getMarketGroupRepository(),
                                                          mRepositoryProvider.getCharacterRepository(),
                                                          this};
        connect(marketAnalysisTab, &MarketAnalysisWidget::updateExternalOrders, this, &MainWindow::updateExternalOrders);
        connect(marketAnalysisTab, &MarketAnalysisWidget::showInEve, this, &MainWindow::showInEve);
        connect(this, &MainWindow::preferencesChanged, marketAnalysisTab, &MarketAnalysisWidget::preferencesChanged);
        connect(this, &MainWindow::preferencesChanged, marketAnalysisTab, &MarketAnalysisWidget::handleNewPreferences);
        addTab(marketAnalysisTab, tr("Market analysis"), TabType::Other);

        QSettings settings;

        const auto actions = mViewTabsMenu->actions();
        for (auto i = 0; i < actions.size(); ++i)
        {
            if (!settings.value(QString{UISettings::tabShowStateKey}.arg(i), true).toBool())
                actions[i]->setChecked(false);
        }

        mViewTabsMenu->addSeparator();

        auto toggleDefaultTabs = [=](bool checked, TabType type) {
            for (const auto action : actions)
            {
                if (static_cast<TabType>(action->data().toInt()) == type)
                    action->setChecked(checked);
            }
        };

        auto action = mViewTabsMenu->addAction(tr("Toggle character tabs"));
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, [=](bool checked) {
            toggleDefaultTabs(checked, TabType::Character);
        });

        action = mViewTabsMenu->addAction(tr("Toggle corporation tabs"));
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, [=](bool checked) {
            toggleDefaultTabs(checked, TabType::Corp);
        });
    }

    void MainWindow::createStatusBar()
    {
        mStatusWalletLabel = new QLabel{this};

        statusBar()->addPermanentWidget(mStatusWalletLabel);
    }

    QWidget *MainWindow::createMainViewTab(QWidget *content)
    {
        auto scroll = new QScrollArea{this};
        scroll->setWidgetResizable(true);
        scroll->setWidget(content);
        scroll->setFrameStyle(QFrame::NoFrame);

        return scroll;
    }

    int MainWindow::addTab(QWidget *widget, const QString &label, TabType type)
    {
        const auto index = mMainTabs->addTab(createMainViewTab(widget), label);
        mTabWidgets[index] = widget;

        auto action = mViewTabsMenu->addAction(label);
        action->setCheckable(true);
        action->setChecked(true);
        action->setData(static_cast<int>(type));
        connect(action, &QAction::toggled, this, [index, this](bool checked) {
            mMainTabs->setTabEnabled(index, checked);
            mMainTabs->setElideMode(mMainTabs->elideMode());    // hack to refresh layout, since there is no api to do that

            QSettings settings;
            settings.setValue(QString{UISettings::tabShowStateKey}.arg(index), checked);
        });

        return index;
    }

    void MainWindow::setUpAutoImportTimer()
    {
        QSettings settings;

        mAutoImportTimer.setInterval(
            settings.value(ImportSettings::autoImportTimeKey, ImportSettings::autoImportTimerDefault).toInt() * 1000 * 60);

        if (settings.value(ImportSettings::autoImportEnabledKey, ImportSettings::autoImportEnabledDefault).toBool())
            mAutoImportTimer.start();
        else
            mAutoImportTimer.stop();
    }

    void MainWindow::refreshCharactersMenu()
    {
        mCharactersMenu->clear();

        const auto &repo = mRepositoryProvider.getCharacterRepository();
        const auto idName = repo.getIdColumn();

        auto characters = repo.getEnabledQuery();
        while (characters.next())
        {
            const auto id = characters.value(idName).value<Character::IdType>();
            auto action = mCharactersMenu->addAction(characters.value("name").toString());

            connect(action, &QAction::triggered, this, [id, this] {
                mMenuWidget->setCurrentCharacter(id);
            });
        }
    }

    template<class T>
    void MainWindow::enumerateEnabledCharacters(T &&func) const
    {
        const auto &repo = mRepositoryProvider.getCharacterRepository();
        const auto idName = repo.getIdColumn();
        auto query = repo.getEnabledQuery();

        while (query.next())
            std::forward<T>(func)(query.value(idName).value<Character::IdType>());
    }
}
