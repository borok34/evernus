#include <QSqlRecord>
#include <QSqlQuery>

#include "ItemRepository.h"

#include "AssetListRepository.h"

namespace Evernus
{
    AssetListRepository::AssetListRepository(const QSqlDatabase &db, const ItemRepository &itemRepository)
        : Repository{db}
        , mItemRepository{itemRepository}
    {
    }

    QString AssetListRepository::getTableName() const
    {
        return "asset_lists";
    }

    QString AssetListRepository::getIdColumn() const
    {
        return "id";
    }

    AssetList AssetListRepository::populate(const QSqlRecord &record) const
    {
        AssetList assetList{record.value("id").value<AssetList::IdType>()};
        assetList.setCharacterId(record.value("character_id").value<Character::IdType>());
        assetList.setNew(false);

        return assetList;
    }

    void AssetListRepository::create(const Repository<Character> &characterRepo) const
    {
        exec(QString{R"(CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY ASC,
            character_id BIGINT NOT NULL REFERENCES %2(%3) ON UPDATE CASCADE ON DELETE CASCADE
        ))"}.arg(getTableName()).arg(characterRepo.getTableName()).arg(characterRepo.getIdColumn()));

        exec(QString{"CREATE UNIQUE INDEX IF NOT EXISTS %1_%2_index ON %1(character_id)"}.arg(getTableName()).arg(characterRepo.getTableName()));
    }

    QStringList AssetListRepository::getColumns() const
    {
        return QStringList{}
            << "id"
            << "character_id";
    }

    void AssetListRepository::bindValues(const AssetList &entity, QSqlQuery &query) const
    {
        query.bindValue(":id", entity.getId());
        query.bindValue(":character_id", entity.getCharacterId());
    }

    void AssetListRepository::preStore(AssetList &entity) const
    {
        if (!entity.isNew())
        {
            auto query = mItemRepository.prepare(QString{"DELETE FROM %1 WHERE asset_list_id = ?"}.arg(mItemRepository.getTableName()));
            query.bindValue(0, entity.getId());

            DatabaseUtils::execQuery(query);
        }
    }

    void AssetListRepository::postStore(AssetList &entity) const
    {
        for (const auto &item : entity)
            item->setListId(entity.getId());

        ItemRepository::PropertyMap map;

        for (const auto &item : entity)
            ItemRepository::fillProperties(*item, map);

        mItemRepository.batchStore(map);
    }
}