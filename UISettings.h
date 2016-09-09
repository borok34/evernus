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
#pragma once

#include <QString>

namespace Evernus
{
    namespace UISettings
    {
        const auto autoCloseTasksDefault = true;
        const auto autoCopyPriceFromInfoDefault = true;
        const auto minimizeToTrayDefault = false;
        const auto minimizeByMarginToolDefault = true;
        const auto translationPath =  "/trans";
        const auto plotNumberFormatDefault = "f";
        const auto usePackagedVolumeDefault = false;
        const auto omitCurrencySymbolDefault = false;
        const auto mainWindowAlwaysOnTopDefault = false;
        const auto useUTCDatesDefault = false;
        const auto combineAssetsDefault = false;
        const auto combineJournalDefault = false;
        const auto combineTransactionsDefault = false;
        const auto columnDelimiterDefault = '\t';
        const auto applyDateFormatToGraphsDefault = false;
        const auto showMarginToolSampleDataDefault = true;

        const auto autoCloseTasksKey = "ui/tasks/autoClose";
        const auto autoCopyPriceFromInfoKey = "ui/price/copyFromInfo";
        const auto marketOrderStateFilterKey = "ui/orders/stateFilter";
        const auto marketOrderPriceStatusFilterKey = "ui/orders/priceStatusFilter";
        const auto minimizeToTrayKey = "ui/mainWindow/minimizeToTray";
        const auto minimizeByMarginToolKey = "ui/mainWindow/minimizeByMarginTool";
        const auto languageKey = "ui/global/language";
        const auto dateTimeFormatKey = "ui/global/dateTimeFormat";
        const auto plotNumberFormatKey = "ui/plot/numberFormat";
        const auto contractStatusFilterKey = "ui/contracts/statusFilter";
        const auto headerStateKey = QStringLiteral("ui/header/state/%1");
        const auto usePackagedVolumeKey = "ui/global/usePackagedVolume";
        const auto tabShowStateKey = "ui/mainWindow/tabShowState/%1";
        const auto tabShowStateParentKey = "ui/mainWindow/tabShowState";
        const auto omitCurrencySymbolKey = "ui/global/omitCurrencySymbol";
        const auto mainWindowAlwaysOnTopKey = "ui/mainWindow/alwaysOnTop";
        const auto useUTCDatesKey = "ui/global/useUTCDates";
        const auto lastCharacterKey = "ui/global/lastCharacter";
        const auto combineAssetsKey = "ui/combine/assets";
        const auto combineJournalKey = "ui/combine/journal";
        const auto combineTransactionsKey = "ui/combine/transactions";
        const auto columnDelimiterKey = "ui/global/columnDelimiter";
        const auto applyDateFormatToGraphsKey = "ui/global/applyDateTimeFormatToGraphs";
        const auto showMarginToolSampleDataKey = "ui/marginTool/showSamples";
    }
}
