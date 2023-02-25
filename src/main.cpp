#include "Utilities.h"

REL::Relocation<Setting*> fActiveEffectConditionUpdateInterval{ REL::ID(553783) };
REL::Relocation<uintptr_t> ptr_EvaluateConditions{ REL::ID(1228998) };
REL::Relocation<uint32_t*> unkCheck{ REL::ID(607340) };
REL::Relocation<float*> fcheckConditionsRate{ REL::ID(1288592) };

void EvaluateConditionsFixed(RE::ActiveEffect* activeEffect, float deltaTime, bool forceUpdate)
{
	if (activeEffect->conditionStatus == ActiveEffect::ConditionStatus::kNotAvailable)
		return;

	if ((activeEffect->flags.all(ActiveEffect::Flags::kHasConditions) || activeEffect->unk70) && activeEffect->magicTarget && activeEffect->magicTarget->GetTargetStatsObject()) {
		if (!forceUpdate) {
			if ((*unkCheck & 1) == 0) {
				*unkCheck |= 1;
				float interval = fActiveEffectConditionUpdateInterval->GetFloat();
				if (interval != 0)
					*fcheckConditionsRate = 1.f / interval;
			}

			if (activeEffect->elapsed <= 0.0F) {
				reinterpret_cast<float&>(activeEffect->pad94) = deltaTime;
				return;
			}

			static const float interval = fActiveEffectConditionUpdateInterval->GetFloat();

			if (reinterpret_cast<float&>(activeEffect->pad94) > 0.0F && reinterpret_cast<float&>(activeEffect->pad94) < interval) {
				reinterpret_cast<float&>(activeEffect->pad94) += deltaTime;
				return;
			}
		}

		TESObjectREFR* target = activeEffect->target.get().get();
		if (activeEffect->effect->conditions.IsTrue(activeEffect->magicTarget->GetTargetStatsObject(), target) && !activeEffect->CheckDisplacementSpellOnTarget())
			activeEffect->conditionStatus = ActiveEffect::ConditionStatus::kTrue;
		else
			activeEffect->conditionStatus = ActiveEffect::ConditionStatus::kFalse;
	} else {
		activeEffect->conditionStatus = ActiveEffect::ConditionStatus::kNotAvailable;
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor"sv);
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical(FMT_STRING("unsupported runtime v{}"), ver.string());
		return false;
	}

	F4SE::AllocTrampoline(8 * 8);

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	F4SE::Trampoline& trampoline = F4SE::GetTrampoline();
	trampoline.write_branch<5>(ptr_EvaluateConditions.address(), &EvaluateConditionsFixed);
	return true;
}
