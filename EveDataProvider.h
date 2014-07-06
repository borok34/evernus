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

#include "EveType.h"

namespace Evernus
{
    class ItemPrice;

    class EveDataProvider
    {
    public:
        EveDataProvider() = default;
        virtual ~EveDataProvider() = default;

        virtual QString getTypeName(EveType::IdType id) const = 0;
        virtual double getTypeVolume(EveType::IdType id) const = 0;
        virtual ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId) const = 0;

        virtual QString getLocationName(quint64 id) const = 0;
    };
}