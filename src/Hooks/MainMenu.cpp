#include "Hooks/MainMenu.hpp"

#include "RE/Offset.hpp"
#include "Settings/GameDelegate.hpp"

namespace Hooks
{
	void MainMenu::Install()
	{
		auto vtbl = REL::Relocation<std::uintptr_t>{ STATIC_OFFSET(MainMenu::Vtbl) };
		_FxDelegateHandler_Accept = vtbl.write_vfunc(1, &FxDelegateHandler_Accept);
	}

	void MainMenu::FxDelegateHandler_Accept(
		RE::MainMenu* a_handler,
		RE::FxDelegateHandler::CallbackProcessor* a_processor)
	{
		_FxDelegateHandler_Accept(a_handler, a_processor);
		a_processor->Process("RequestNewGameOptions", &Settings::RequestNewGameOptions);
		a_processor->Process("OptionChange", &Settings::OptionChange);
		a_processor->Process("SaveSettings", &Settings::SaveSettings);
	}
}
