#pragma once

#include <functional>
#include <vector>

#include "APIResponseCache.h"
#include "APIInterface.h"
#include "AssetList.h"
#include "Character.h"

namespace Evernus
{
    class APIManager
        : public QObject
    {
        Q_OBJECT

    public:
        template<class T>
        using Callback = std::function<void (const T &data, const QString &error)>;

        typedef std::vector<Character::IdType> CharacterList;

        APIManager();
        virtual ~APIManager() = default;

        void fetchCharacterList(const Key &key, const Callback<CharacterList> &callback) const;
        void fetchCharacter(const Key &key, Character::IdType characterId, const Callback<Character> &callback) const;
        void fetchAssets(const Key &key, Character::IdType characterId, const Callback<AssetList> &callback) const;

        QDateTime getCharacterLocalCacheTime(Key::IdType key, Character::IdType characterId) const;
        QDateTime getAssetsLocalCacheTime(Key::IdType key, Character::IdType characterId) const;

    signals:
        void generalError(const QString &info);

    private:
        static const QString eveTimeFormat;

        mutable APIResponseCache mCache;
        APIInterface mInterface;


        template<class T, class CurElem>
        static std::vector<T> parseResults(const QString &xml, const QString &rowsetName);
        template<class T>
        static T parseResult(const QString &xml);

        static QString queryPath(const QString &path, const QString &xml);

        static QDateTime getCachedUntil(const QString &xml);
        static void handlePotentialError(const QString &xml, const QString &error);
    };
}
