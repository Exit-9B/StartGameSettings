#include "Settings/GameDelegate.hpp"

#include "RE/Offset.hpp"
#include "Settings/Collection.hpp"
#include "Settings/StaticCollection.hpp"

static constexpr bool DeferOptionChange = true;

static constexpr double ID_NewGame0 = 0;

namespace Settings
{
	struct HelpSetter
	{
		RE::GFxMovieView* movie;
		RE::GFxValue& entryObject;

		void operator()(std::monostate const&) const {}
		void operator()(std::string const& help) const
		{
			entryObject.SetMember("help", help.c_str());
		}
		void operator()(std::vector<std::string> const& help) const
		{
			RE::GFxValue items;
			movie->CreateArray(&items);
			for (const auto& str : help) {
				items.PushBack(str.c_str());
			}
			entryObject.SetMember("help", items);
		}
	};

	struct EntryDataSetter
	{
		RE::GFxMovieView* movie;
		RE::GFxValue& entryObject;

		void operator()(ItemA<Type::slider> const& item) const
		{
			entryObject.SetMember("movieType", 0);
			entryObject.SetMember("text", item.text.c_str());
			std::visit(HelpSetter{ movie, entryObject }, item.help);

			if (const auto& valueOptions = item.valueOptions) {
				entryObject.SetMember("defaultVal", item.valueOptions->defaultValue);

				entryObject.SetMember("minimum", item.valueOptions->min);
				entryObject.SetMember("maximum", item.valueOptions->max);
				entryObject.SetMember("interval", item.valueOptions->step);
			}
		}

		void operator()(ItemA<Type::stepper> const& item) const
		{
			entryObject.SetMember("movieType", 1);
			entryObject.SetMember("text", item.text.c_str());
			std::visit(HelpSetter{ movie, entryObject }, item.help);

			if (const auto& valueOptions = item.valueOptions) {
				entryObject.SetMember("defaultVal", item.valueOptions->defaultValue);

				RE::GFxValue options;
				movie->CreateArray(&options);
				for (const auto& option : item.valueOptions->options) {
					options.PushBack(option.c_str());
				}
				entryObject.SetMember("options", options);
			}
		}

		void operator()(ItemA<Type::toggle> const& item) const
		{
			entryObject.SetMember("movieType", 2);
			entryObject.SetMember("text", item.text.c_str());
			std::visit(HelpSetter{ movie, entryObject }, item.help);

			if (const auto& valueOptions = item.valueOptions) {
				entryObject.SetMember("defaultVal", item.valueOptions->defaultValue);
			}
		}
	};

	struct GroupControlInserter
	{
		GroupControlStore& groupControls;
		double value;

		void operator()(std::monostate const&) const {}

		void operator()(group_t group) const { InsertGroup(groupControls, group, !!value); }

		void operator()(std::vector<std::optional<group_t>> const& groups) const
		{
			for (const auto& [i, group] : std::views::enumerate(groups)) {
				if (group) {
					InsertGroup(groupControls, *group, static_cast<std::ptrdiff_t>(value) == i);
				}
			}
		}
	};

	[[nodiscard]] static const Item* QueryID(double ID)
	{
		if (ID >= ID_NewGame0) {
			std::size_t index = static_cast<std::size_t>(ID - ID_NewGame0);
			if (index < StaticCollection::Instance.NewGame.size()) {
				return std::addressof(StaticCollection::Instance.NewGame[index]);
			}
		}

		return nullptr;
	}

	[[nodiscard]] static bool IsPrefSetting(double ID)
	{
		if (const auto item = QueryID(ID)) {
			return item->sourceType() == SourceType::INIPrefSetting;
		}

		return true;
	}

	[[nodiscard]] static bool HasGroupControl(const Item& item)
	{
		const GroupControl& groupControl = item.groupControl();
		return !std::holds_alternative<std::monostate>(groupControl);
	}

	[[nodiscard]] static bool HasGroupControl(double ID)
	{
		if (const auto item = QueryID(ID)) {
			return HasGroupControl(*item);
		}
		return false;
	}

	static void StoreValue(double ID, double value)
	{
		if (const auto item = QueryID(ID)) {
			item->StoreValue(value);
			return;
		}

		logger::error("Failed to resolve option ID: {}"sv, ID);
	}

	static void AddOptions(
		RE::GFxMovieView* movie,
		RE::GFxValue& entryList,
		const std::vector<Item>& items,
		const std::vector<std::size_t>& pages)
	{
		assert(entryList.IsArray());

		std::map<double, double> oldValues;
		for (const auto i : std::views::iota(0U, entryList.GetArraySize())) {
			RE::GFxValue entryObject;
			entryList.GetElement(i, &entryObject);
			if (entryObject.IsObject()) {
				RE::GFxValue ID, value;
				entryObject.GetMember("ID", &ID);
				entryObject.GetMember("value", &value);
				if (ID.IsNumber() && value.IsNumber()) {
					oldValues[ID.GetNumber()] = value.GetNumber();
				}
			}
		}
		entryList.ClearElements();

		std::size_t begin = 0;
		for (const std::size_t sentinel : pages) {
			GroupControlStore groupControls;
			std::vector<double> values;
			values.reserve(sentinel - begin);

			for (const auto i : std::views::iota(begin, sentinel)) {
				const auto& item = items[i];

				const GroupControl& groupControl = item.groupControl();

				double value;
				if (const auto it = oldValues.find(ID_NewGame0 + i); it != oldValues.end()) {
					value = it->second;
				}
				else {
					value = item.InitialValue();
				}
				values.emplace_back(value);

				std::visit(GroupControlInserter{ groupControls, value }, groupControl);
			}

			for (const auto i : std::views::iota(begin, sentinel)) {
				const auto& item = items[i];

				if (!item.IsActive(groupControls)) {
					continue;
				}

				RE::GFxValue entryObject;
				movie->CreateObject(&entryObject);
				std::visit(EntryDataSetter{ movie, entryObject }, item.variant);
				entryObject.SetMember("ID", ID_NewGame0 + i);
				entryObject.SetMember("value", values[i - begin]);
				entryList.PushBack(entryObject);
			}

			begin = sentinel;
		}
	}

	void RequestNewGameOptions(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 1) {
			logger::error(
				"RequestNewGameOptions: Expected 1 argument, received {}",
				a_params.GetArgCount());
			return;
		}

		RE::GFxMovieView* const movie = a_params.GetMovie();
		if (!movie) {
			return;
		}

		RE::GFxValue entryList = a_params[0];

		if (!entryList.IsArray()) {
			logger::error("RequestNewGameOptions: Expected [Array]");
			return;
		}

		AddOptions(
			movie,
			entryList,
			StaticCollection::Instance.NewGame,
			StaticCollection::NewGamePages);
	}

	void OptionChange(const RE::FxDelegateArgs& a_params)
	{
		if (a_params.GetArgCount() < 2) {
			logger::error(
				"OptionChange: Expected 2 arguments, received {}",
				a_params.GetArgCount());
			return;
		}

		RE::GFxValue ID = a_params[0];
		RE::GFxValue value = a_params[1];

		if (!ID.IsNumber() || !value.IsNumber()) {
			logger::error("OptionChange: Expected [Number, Number]", a_params.GetArgCount());
			return;
		}

		if constexpr (!DeferOptionChange) {
			StoreValue(ID.GetNumber(), value.GetNumber());
		}

		RE::GFxMovieView* const movie = a_params.GetMovie();
		if (!movie) {
			return;
		}

		if (IsPrefSetting(ID.GetNumber())) {
			movie->SetVariable(
				"_root.MenuHolder.Menu_mc.bPrefsChanged",
				true,
				RE::GFxMovie::SetVarType::kNormal);
		}

		if (HasGroupControl(ID.GetNumber())) {
			RE::FxResponseArgs<0> response{};
			RE::FxDelegate::Invoke(movie, "RefreshSettingsList", response);
		}
	}

	void SaveSettings(const RE::FxDelegateArgs& a_params)
	{
		RE::GFxMovieView* const movie = a_params.GetMovie();
		if (!movie) {
			return;
		}

		if constexpr (DeferOptionChange) {
			RE::GFxValue entryList;
			movie->GetVariable(
				&entryList,
				"_root.MenuHolder.Menu_mc.SettingsPanel_mc.List_mc.entryList");
			if (!entryList.IsArray()) {
				return;
			}

			for (const auto i : std::views::iota(0U, entryList.GetArraySize())) {
				RE::GFxValue entryObject;
				entryList.GetElement(i, &entryObject);
				if (!entryObject.IsObject()) {
					continue;
				}

				RE::GFxValue ID, value;
				entryObject.GetMember("ID", &ID);
				entryObject.GetMember("value", &value);
				if (!ID.IsNumber() || !value.IsNumber()) {
					return;
				}

				StoreValue(ID.GetNumber(), value.GetNumber());
			}
		}

		RE::GFxValue bPrefsChanged;
		if (movie->GetVariable(&bPrefsChanged, "_root.MenuHolder.Menu_mc.bPrefsChanged")) {
			if (bPrefsChanged.IsBool() && bPrefsChanged.GetBool()) {
				const auto prefSettings = RE::INIPrefSettingCollection::GetSingleton();
				if (prefSettings && prefSettings->OpenHandle(true)) {
					prefSettings->WriteSettings();
					prefSettings->CloseHandle();
				}
			}
		}

		RE::FxResponseArgs<0> response;
		RE::FxDelegate::Invoke(movie, "SettingsSaved", response);
	}
}
