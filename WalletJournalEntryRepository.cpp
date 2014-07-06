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
#include <QSqlRecord>
#include <QSqlQuery>

#include "WalletJournalEntryRepository.h"

namespace Evernus
{
    QString WalletJournalEntryRepository::getTableName() const
    {
        return "wallet_journal";
    }

    QString WalletJournalEntryRepository::getIdColumn() const
    {
        return "id";
    }

    WalletJournalEntry WalletJournalEntryRepository::populate(const QSqlRecord &record) const
    {
        const auto argName = record.value("arg_name");
        const auto reason = record.value("reason");
        const auto taxReceiverId = record.value("tax_receiver_id");
        const auto taxAmount = record.value("tax_amount");

        WalletJournalEntry walletJournalEntry{record.value("id").value<WalletJournalEntry::IdType>()};
        walletJournalEntry.setCharacterId(record.value("character_id").value<Character::IdType>());
        walletJournalEntry.setTimestamp(record.value("timestamp").toDateTime());
        walletJournalEntry.setRefTypeId(record.value("ref_type_id").toUInt());
        walletJournalEntry.setOwnerName1(record.value("owner_name_1").toString());
        walletJournalEntry.setOwnerId1(record.value("owner_id_1").toULongLong());
        walletJournalEntry.setOwnerName2(record.value("owner_name_2").toString());
        walletJournalEntry.setOwnerId2(record.value("owner_id_2").toULongLong());
        walletJournalEntry.setArgName((argName.isNull()) ? (WalletJournalEntry::ArgType{}) : (argName.toString()));
        walletJournalEntry.setArgId(record.value("arg_id").toULongLong());
        walletJournalEntry.setAmount(record.value("amount").toDouble());
        walletJournalEntry.setBalance(record.value("balance").toDouble());
        walletJournalEntry.setReason((reason.isNull()) ? (WalletJournalEntry::ReasonType{}) : (reason.toString()));
        walletJournalEntry.setTaxReceiverId((taxReceiverId.isNull()) ? (WalletJournalEntry::TaxReceiverType{}) : (taxReceiverId.toULongLong()));
        walletJournalEntry.setTaxAmount((taxAmount.isNull()) ? (WalletJournalEntry::TaxAmountType{}) : (taxAmount.toDouble()));
        walletJournalEntry.setNew(false);

        return walletJournalEntry;
    }

    void WalletJournalEntryRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE,
            timestamp DATETIME NOT NULL,
            ref_type_id INTEGER NOT NULL,
            owner_name_1 TEXT NOT NULL,
            owner_id_1 BIGINT NOT NULL,
            owner_name_2 TEXT NOT NULL,
            owner_id_2 BIGINT NOT NULL,
            arg_name TEXT NULL,
            arg_id BIGINT NOT NULL,
            amount NUMERIC NOT NULL,
            balance NUMERIC NOT NULL,
            reason TEXT NULL,
            tax_receiver_id BIGINT NULL,
            tax_amount NUMERIC NULL
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
    }

    QStringList WalletJournalEntryRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id"
            << "timestamp"
            << "ref_type_id"
            << "owner_name_1"
            << "owner_id_1"
            << "owner_name_2"
            << "owner_id_2"
            << "arg_name"
            << "arg_id"
            << "amount"
            << "balance"
            << "reason"
            << "tax_receiver_id"
            << "tax_amount";
    }

    void WalletJournalEntryRepository::bindValues(const WalletJournalEntry &entity, QSqlQuery &query) const
    {
        const auto argName = entity.getArgName();
        const auto reason = entity.getReason();
        const auto taxReceiverId = entity.getTaxReceiverId();
        const auto taxAmount = entity.getTaxAmount();

        if (entity.getId() != WalletJournalEntry::invalidId)
            query.bindValue(":id", entity.getId());

        query.bindValue(":character_id", entity.getCharacterId());
        query.bindValue(":timestamp", entity.getTimestamp());
        query.bindValue(":ref_type_id", entity.getRefTypeId());
        query.bindValue(":owner_name_1", entity.getOwnerName1());
        query.bindValue(":owner_id_1", entity.getOwnerId1());
        query.bindValue(":owner_name_2", entity.getOwnerName2());
        query.bindValue(":owner_id_2", entity.getOwnerId2());
        query.bindValue(":argName", (argName) ? (*argName) : (QVariant{QVariant::String}));
        query.bindValue(":arg_id", entity.getArgId());
        query.bindValue(":amount", entity.getAmount());
        query.bindValue(":balance", entity.getBalance());
        query.bindValue(":reason", (reason) ? (*reason) : (QVariant{QVariant::String}));
        query.bindValue(":tax_receiver_id", (taxReceiverId) ? (*taxReceiverId) : (QVariant{QVariant::ULongLong}));
        query.bindValue(":tax_amount", (taxAmount) ? (*taxAmount) : (QVariant{QVariant::Double}));
    }

    void WalletJournalEntryRepository::bindPositionalValues(const WalletJournalEntry &entity, QSqlQuery &query) const
    {
        const auto argName = entity.getArgName();
        const auto reason = entity.getReason();
        const auto taxReceiverId = entity.getTaxReceiverId();
        const auto taxAmount = entity.getTaxAmount();

        if (entity.getId() != WalletJournalEntry::invalidId)
            query.addBindValue(entity.getId());

        query.addBindValue(entity.getCharacterId());
        query.addBindValue(entity.getTimestamp());
        query.addBindValue(entity.getRefTypeId());
        query.addBindValue(entity.getOwnerName1());
        query.addBindValue(entity.getOwnerId1());
        query.addBindValue(entity.getOwnerName2());
        query.addBindValue(entity.getOwnerId2());
        query.addBindValue((argName) ? (*argName) : (QVariant{QVariant::String}));
        query.addBindValue(entity.getArgId());
        query.addBindValue(entity.getAmount());
        query.addBindValue(entity.getBalance());
        query.addBindValue((reason) ? (*reason) : (QVariant{QVariant::String}));
        query.addBindValue((taxReceiverId) ? (*taxReceiverId) : (QVariant{QVariant::ULongLong}));
        query.addBindValue((taxAmount) ? (*taxAmount) : (QVariant{QVariant::Double}));
    }
}