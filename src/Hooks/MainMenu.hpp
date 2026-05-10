#pragma once

namespace Hooks
{
	class MainMenu final
	{
	public:
		static void Install();

	private:
		static void FxDelegateHandler_Accept(
			RE::MainMenu* a_handler,
			RE::FxDelegateHandler::CallbackProcessor* a_processor);

		inline static REL::Relocation<decltype(FxDelegateHandler_Accept)>
			_FxDelegateHandler_Accept;
	};
}
