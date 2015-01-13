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

namespace Evernus
{
    namespace MarketAnalysisSettings
    {
        const auto dontSaveLargeOrdersDefault = true;
        const auto smaDaysDefault = 20;
        const auto macdFastDaysDefault = 5;
        const auto macdSlowDaysDefault = 15;
        const auto macdEmaDaysDefault = 5;
        const auto showLegendDefault = true;

        const auto dontSaveLargeOrdersKey = "marketAnalysis/dontSaveOrders";
        const auto minVolumeFilterKey = "marketAnalysis/filter/minVolume";
        const auto maxVolumeFilterKey = "marketAnalysis/filter/maxVolume";
        const auto minMarginFilterKey = "marketAnalysis/filter/minMargin";
        const auto maxMarginFilterKey = "marketAnalysis/filter/maxMargin";
        const auto smaDaysKey = "marketAnalysis/smaDays";
        const auto lastRegionKey = "marketAnalysis/lastRegion";
        const auto macdFastDaysKey = "marketAnalysis/macd/fastDays";
        const auto macdSlowDaysKey = "marketAnalysis/macd/slowDays";
        const auto macdEmaDaysKey = "marketAnalysis/macd/emaDays";
        const auto showLegendKey = "marketAnalysis/showLegend";
    }
}