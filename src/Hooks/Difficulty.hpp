#pragma once

namespace Hooks
{
	class Difficulty final
	{
	public:
		static void Install();

		static bool SetGlobal(RE::TESGlobal* global);

		static void OnNewGame();

		static void OnLoadGame();

	private:
		static void SetDifficultyHook();

		inline static RE::TESGlobal* _CharacterDifficulty{ nullptr };
		inline static REL::Relocation<std::int32_t*> _iDifficultySetting;
	};
}
