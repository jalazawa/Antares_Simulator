/*
** Copyright 2007-2018 RTE
** Authors: Antares_Simulator Team
**
** This file is part of Antares_Simulator.
**
** Antares_Simulator is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** There are special exceptions to the terms and conditions of the
** license as they are applied to this software. View the full text of
** the exceptions in file COPYING.txt in the directory of this software
** distribution
**
** Antares_Simulator is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Antares_Simulator. If not, see <http://www.gnu.org/licenses/>.
**
** SPDX-License-Identifier: licenceRef-GPL3_WITH_RTE-Exceptions
*/

#include "rules.h"
#include "../study.h"
#include "../../logs.h"


using namespace Yuni;


namespace Antares
{
namespace Data
{
namespace ScenarioBuilder
{

	Rules::Rules() :
		load(timeSeriesLoad),
		solar(timeSeriesSolar),
		hydro(timeSeriesHydro),
		wind(timeSeriesWind),
		thermal(nullptr),
		pAreaCount(0)
	{
	}


	Rules::~Rules()
	{
		delete[] thermal;
	}



	bool Rules::loadFromStudy(const Study& study)
	{
		logs.info() << "  Loading data for the scenario builder overlay";
		load.loadFromStudy(study);
		solar.loadFromStudy(study);
		hydro.loadFromStudy(study);
		wind.loadFromStudy(study);

		delete[] thermal;
		pAreaCount = study.areas.size();
		thermal = new TSNumberRules[pAreaCount];
		for (uint i = 0; i != pAreaCount; ++i)
		{
			thermal[i].attachArea(study.areas.byIndex[i]);
			thermal[i].loadFromStudy(study);
		}
		return true;
	}


	void Rules::saveToINIFile(const Study& study, Yuni::IO::File::Stream& file) const
	{
		file << "[" << pName << "]\n";
		if (pAreaCount)
		{
			// load
			load.saveToINIFile(study, file);
			// solar
			solar.saveToINIFile(study, file);
			// hydro
			hydro.saveToINIFile(study, file);
			// wind
			wind.saveToINIFile(study, file);
			// Thermal, each area
			for (uint i = 0; i != pAreaCount; ++i)
				thermal[i].saveToINIFile(study, file);
		}
		file << '\n';
	}


	bool Rules::reset(const Study& study)
	{
		// Alias to the current study
		assert(&study != nullptr);
		pStudy = &study;

		// The new area count
		pAreaCount = study.areas.size();

		load.reset(study);
		solar.reset(study);
		hydro.reset(study);
		wind.reset(study);

		delete[] thermal;
		thermal = new TSNumberRules[pAreaCount];

		for (uint i = 0; i != pAreaCount; ++i)
		{
			thermal[i].attachArea(study.areas.byIndex[i]);
			thermal[i].reset(study);
		}
		return true;
	}


	void Rules::loadFromInstrs(const Study& study, const AreaName::Vector& instrs, uint value, bool updaterMode=false)
	{
		assert(instrs.size() > 2);

		const AreaName& kind = instrs[0];
		if (kind.size() != 1)
			return;
		const char kindc = kind[0];

		// The current area name
		const AreaName& areaname = instrs[1];
		const Data::Area* area = study.areas.find(areaname);
		if (!area)
		{
			// silently ignore the error
			if (not updaterMode)
				logs.warning() << "[scenario-builder] The area '" << areaname << "' has not been found";
			
			return;
		}
		// Year
		const uint year = instrs[2].to<uint>();

		switch (kindc)
		{
			case 't':
				{
					if (instrs.size() != 4)
						return;
					const ThermalClusterName& clustername = instrs[3];
					const ThermalCluster* cluster = area->thermal.list.find(clustername);
					if (not cluster)
						cluster = area->thermal.mustrunList.find(clustername);

					if (cluster)
					{
						thermal[area->index].set(cluster->areaWideIndex, year, value);
					}
					else
						if (not updaterMode) 
							logs.warning() << "cluster " << area->id << " / " << clustername << " (size:" << clustername.size() << ") not found: it may not exist or it is disabled";
					break;
				}
			case 'l':
				load.set(area->index, year, value);
				return;
			case 'w':
				wind.set(area->index, year, value);
				return;
			case 'h':
				hydro.set(area->index, year, value);
				return;
			case 's':
				solar.set(area->index, year, value);
				return;
		}
		return;
	}



	void Rules::apply(const Study& study)
	{
		if (pAreaCount)
		{
			load.apply(study);
			solar.apply(study);
			hydro.apply(study);
			wind.apply(study);
			for (uint i = 0; i != pAreaCount; ++i)
				thermal[i].apply(study);
		}
	}




} // namespace ScenarioBuilder
} // namespace Data
} // namespace Antares


