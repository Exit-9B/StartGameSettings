#pragma once

#include "Settings/Collection.hpp"

namespace Settings::StaticCollection
{
	extern Collection Instance;
	extern std::vector<std::size_t> NewGamePages;

	void Load();
}
