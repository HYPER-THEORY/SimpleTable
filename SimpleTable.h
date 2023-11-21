/**
 * Copyright (C) 2023 Hypertheory
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <array>
#include <iomanip>
#include <sstream>
#include <variant>
#include <vector>

template <typename... Types>
class SimpleTable {
public:
	using array_string = std::array<std::string, sizeof...(Types)>;
	using array_size_t = std::array<size_t, sizeof...(Types)>;

	SimpleTable(std::ios_base::fmtflags flags = 0, int precision = 6) {
		stream.setf(flags);
		stream.precision(precision);
	}

	template <typename... StringTypes, typename = std::enable_if_t<
		(sizeof...(StringTypes) == sizeof...(Types)) &&
		(std::is_convertible_v<StringTypes, std::string> && ...)>>
	inline void add(StringTypes&&... row) noexcept {
		rows.emplace_back(array_string{std::forward<StringTypes>(row)...});
	}

	inline void add(Types&&... row) noexcept {
		rows.emplace_back(std::forward_as_tuple(row...));
	}

	inline void add() noexcept {
		rows.emplace_back(std::monostate());
	}

	[[nodiscard]] inline std::string get() const noexcept {
		array_size_t widths = {};

		auto compute_widths = [&](auto&&... value) -> void {
			size_t index = 0, pos = 0;
			((pos = (stream << value).tellp(), stream.seekp(0),
				pos > widths[index] ? widths[index++] = pos : index++
			), ...);
		};

		auto apply_compute_widths = [&](auto&& value) -> void {
			using value_t = std::decay_t<decltype(value)>;
			if constexpr (!std::is_same_v<value_t, std::monostate>) {
				std::apply(compute_widths, value);
			}
		};
		
		std::string separator, padding;

		auto streamify_row = [&](auto&&... value) -> void {
			size_t index = 0;
			stream << separator << padding;
			((stream << std::setw(widths[index++]) << value << padding <<
				separator << (index == sizeof...(Types) ? "\n" : padding)
			), ...);
		};

		auto apply_streamify_row = [&](auto&& value) -> void {
			using value_t = std::decay_t<decltype(value)>;
			if constexpr (!std::is_same_v<value_t, std::monostate>) {
				separator = "|", padding = " ";
				stream.fill(' ');
				std::apply(streamify_row, value);
			} else {
				separator = "+", padding = "-";
				stream.fill('-');
				std::apply(streamify_row, array_string());
			}
		};

		stream.str({});

		for (auto& row : rows) std::visit(apply_compute_widths, row);
		
		apply_streamify_row(std::monostate());
		for (auto& row : rows) std::visit(apply_streamify_row, row);
		apply_streamify_row(std::monostate());

		return stream.str();
	}

private:
	mutable std::ostringstream stream;
	std::vector<std::variant<array_string, std::tuple<Types...>, std::monostate>> rows;
};
