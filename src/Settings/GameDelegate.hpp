#pragma once

namespace Settings
{
	void RequestNewGameOptions(const RE::FxDelegateArgs& a_params);

	void OptionChange(const RE::FxDelegateArgs& a_params);

	void SaveSettings(const RE::FxDelegateArgs& a_params);
}
