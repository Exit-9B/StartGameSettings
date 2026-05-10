#include "Hooks/Difficulty.hpp"

#include "RE/Offset.hpp"

#include <xbyak/xbyak.h>

#pragma section(".jit", execute)

namespace Hooks
{
	void Difficulty::Install()
	{
		SetDifficultyHook();
	}

	bool Difficulty::SetGlobal(RE::TESGlobal* global)
	{
		if (!_iDifficultySetting.get()) {
			logger::error("Difficulty hook is not active"sv);
			return false;
		}

		_CharacterDifficulty = global;

		if (_CharacterDifficulty) {
			_CharacterDifficulty->value = static_cast<float>(*_iDifficultySetting);
		}
		return true;
	}

	void Difficulty::OnNewGame()
	{
		if (_CharacterDifficulty) {
			_CharacterDifficulty->value = static_cast<float>(*_iDifficultySetting);
		}
	}

	[[nodiscard]] static std::int32_t DifficultyLevelMax()
	{
		REL::Relocation<RE::Setting*> iDifficultyLevelMax{ STATIC_OFFSET(iDifficultyLevelMax) };
		return (iDifficultyLevelMax->GetSInt() >= 5) ? 5 : 4;
	}

	static void SetDifficulty(RE::PlayerCharacter* player, std::int32_t difficulty)
	{
		if (difficulty >= 0 && difficulty <= DifficultyLevelMax()) {
			player->difficulty = difficulty;
		}
	}

	void Difficulty::OnLoadGame()
	{
		if (const auto player = RE::PlayerCharacter::GetSingleton();
			player && _CharacterDifficulty)
		{
			const auto difficulty = static_cast<std::int32_t>(_CharacterDifficulty->value);
			SetDifficulty(player, difficulty);
		}
	}

	void Difficulty::SetDifficultyHook()
	{
		const std::ptrdiff_t offset =
			REL::Module::get().version() >= SKSE::RUNTIME_1_6_1130 ? 0x1B5 : 0x18D;

		REL::Relocation<std::byte*> hook{ STATIC_OFFSET(JournalSettings::OnOptionChange) +
										  offset };

		if (!REL::make_pattern<"89 15">().match(hook.address())) {
			logger::error("Failed to install SetDifficulty hook"sv);
			return;
		}

		__declspec(allocate(".jit")) alignas(16) static constinit std::array<std::uint8_t, 32>
			buffer{ REL::RET };
		_iDifficultySetting = SKSE::GetTrampoline().write_call<6>(hook.address(), buffer.data());

		struct Code : Xbyak::CodeGenerator
		{
			Code() : Xbyak::CodeGenerator(buffer.size(), buffer.data())
			{
				Xbyak::Label original;

				mov(rax, ptr[rip + std::addressof(_CharacterDifficulty)]);
				test(rax, rax);
				jz(original, T_SHORT);

				cvtsi2ss(xmm0, edx);
				movss(dword[rax + offsetof(RE::TESGlobal, value)], xmm0);
				ret();

				L(original);
				mov(rax, ptr[rip + std::addressof(_iDifficultySetting)]);
				mov(dword[rax], edx);
				ret();
			}
		};

		if (auto ctx = REL::safe_write_context(buffer.data(), buffer.size())) {
			Code().ready();
		}
	}
}
