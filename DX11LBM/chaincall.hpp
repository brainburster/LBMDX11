#ifndef _CHAINCALL_HPP
#define _CHAINCALL_HPP

#include <tuple>
#include <type_traits>
#include <functional>

namespace chaincall
{
	namespace helper
	{
		namespace cpp11
		{
			template <size_t...>
			struct index_sequence
			{
			};

			template <size_t N, size_t... M>
			struct make_index_sequence : make_index_sequence<N - 1, N - 1, M...>
			{
			};

			template <size_t... M>
			struct make_index_sequence<0, M...> : index_sequence<M...>
			{
			};
		} // namespace cpp11

		template <typename F, typename T, size_t... I>
		inline decltype(auto) _apply_impl(F&& f, T&& t, cpp11::index_sequence<I...>)
		{
			return f(std::get<I>(std::forward<T>(t))...);
		}

		template <typename F, typename... Args, typename Indices = cpp11::make_index_sequence<sizeof...(Args)>>
		inline decltype(auto) _apply(F&& f, std::tuple<Args...>&& t)
		{
			return _apply_impl(std::forward<F>(f), std::move(t), Indices{});
		}
	} // namespace helper

	namespace impl
	{
		template <typename T>
		struct Chain
		{
			T value;
		};

		template <>
		struct Chain<void>
		{
		};

		template <typename RHS, typename ParamType>
		inline auto operator>>(Chain<ParamType>&& lhs, RHS&& rhs) -> typename std::enable_if<!std::is_void<decltype(rhs(std::move(lhs.value)))>::value, Chain<decltype(rhs(std::move(lhs.value)))>>::type
		{
			auto temp = std::forward<RHS>(rhs)(reinterpret_cast<typename std::decay<decltype(lhs.value)>::type&&>(*reinterpret_cast<typename std::decay<decltype(lhs.value)>::type*>(&lhs)));
			return std::move(*reinterpret_cast<Chain<decltype(temp)> *>(&temp));
		};

		template <typename RHS, typename ParamType>
		inline auto operator>>(Chain<ParamType>&& lhs, RHS&& rhs) -> typename std::enable_if<std::is_void<decltype(rhs(std::move(lhs.value)))>::value, Chain<void>>::type
		{
			std::forward<RHS>(rhs)(reinterpret_cast<typename std::decay<decltype(lhs.value)>::type&&>(*reinterpret_cast<typename std::decay<decltype(lhs.value)>::type*>(&lhs)));
			return Chain<void>{};
		};

		template <typename RHS, typename ParamType>
		inline auto operator>>(Chain<ParamType>&& lhs, RHS&& rhs) -> typename std::enable_if<
			!std::is_bind_expression_v<std::decay_t<RHS>> &&
			!std::is_void<std::result_of_t<RHS()>>::value,
			Chain<std::result_of_t<RHS()> >>::type
		{
			auto temp = std::forward<RHS>(rhs)();
			return std::move(*reinterpret_cast<Chain<decltype(temp)> *>(&temp));
		};

		template <typename RHS, typename ParamType>
		inline auto operator>>(Chain<ParamType>&& lhs, RHS&& rhs) -> typename std::enable_if<
			!std::is_bind_expression_v<std::decay_t<RHS>>&&
			std::is_void<std::result_of_t<RHS()>>::value,
			Chain<void>>::type
		{
			std::forward<RHS>(rhs)();
			return Chain<void>{};
		};

		template <typename RHS, typename ParamType>
		inline auto operator>>(Chain<ParamType>&& lhs, RHS&& rhs) -> typename std::enable_if<!std::is_void<decltype(helper::_apply(rhs, std::move(lhs.value)))>::value, Chain<decltype(helper::_apply(rhs, std::move(lhs.value)))>>::type
		{
			auto temp = helper::_apply(std::forward<RHS>(rhs), std::move(lhs.value));
			return std::move(*reinterpret_cast<Chain<decltype(temp)> *>(&temp));
		};

		template <typename RHS, typename ParamType>
		inline auto operator>>(Chain<ParamType>&& lhs, RHS&& rhs)-> typename std::enable_if<std::is_void<decltype(helper::_apply(rhs, std::move(lhs.value)))>::value, Chain<void>>::type
		{
			helper::_apply(std::forward<RHS>(rhs), std::move(lhs.value));
			return Chain<void>{};
		};
	} // namespace impl

	template <typename T>
	inline impl::Chain<T> chain(T&& value)
	{
		return std::move(*reinterpret_cast<impl::Chain<T> *>(&value));
	};

	template <typename... T>
	inline impl::Chain<std::tuple<T...>> chain(T &&...value)
	{
		return { std::forward_as_tuple(std::forward<T>(value)...) };
	};

	inline impl::Chain<void> chain()
	{
		return impl::Chain<void>{};
	}

	namespace impl
	{
		template <typename... FuncList>
		struct Pipe_impl// : std::enable_if<helper::ChainAbleHelper<FuncList...>::is_chainable, Pipe_impl<>>::type
		{
			std::tuple<FuncList...> funcs;

			template <typename T1, typename T2>
			inline auto __pipe_impl(T1&& t1, T2&& t2) -> decltype(std::forward<T1>(t1) >> std::forward<T2>(t2))
			{
				return std::forward<T1>(t1) >> std::forward<T2>(t2);
			}

			template <typename T1, typename T2, typename... Args>
			inline auto __pipe_impl(T1&& t1, T2&& t2, Args &&...args) -> decltype(__pipe_impl(std::forward<T1>(t1) >> std::forward<T2>(t2), std::forward<Args>(args)...))
			{
				return __pipe_impl(std::forward<T1>(t1) >> std::forward<T2>(t2), std::forward<Args>(args)...);
			}

			template <typename CHAIN, size_t... I>
			inline auto _pipe_impl(CHAIN&& t1, helper::cpp11::index_sequence<I...>) -> decltype(__pipe_impl(std::forward<CHAIN>(t1), std::get<I>(funcs)...))
			{
				//return (std::forward<CHAIN>(t1) >> ... >> std::get<I>(funcs)); //c++17
				return __pipe_impl(std::forward<CHAIN>(t1), std::get<I>(funcs)...);
			}

			template <typename... Args, typename Indices = helper::cpp11::make_index_sequence<sizeof...(FuncList)>>
			inline auto operator()(Args &&...args) -> decltype(_pipe_impl(chain(std::forward<Args>(args)...), Indices{}))
			{
				return _pipe_impl(chain(std::forward<Args>(args)...), Indices{});
			}
		};

		template <>
		struct Pipe_impl<>
		{
			//std::tuple<> funcs;
		};

		template <typename RHS>
		inline auto operator>>(Pipe_impl<>&& p, RHS&& f) -> typename std::enable_if<
			true,
			Pipe_impl<RHS>>::type
		{
			auto wraped_func = std::forward_as_tuple(std::forward<RHS>(f));
			return std::move(*reinterpret_cast<Pipe_impl<RHS> *>(&wraped_func));
		}

			template <typename RHS, typename... FuncList>
		inline auto operator>>(Pipe_impl<FuncList...>&& p, RHS&& f) -> typename std::enable_if<
			true,
			Pipe_impl<FuncList..., RHS>>::type
		{
			auto funcs = std::tuple_cat(std::move(p.funcs), std::forward_as_tuple(std::forward<RHS>(f)));
			return std::move(*reinterpret_cast<Pipe_impl<FuncList..., RHS> *>(&funcs));
		}

			template <typename RHS>
		inline auto operator<<(Pipe_impl<>&& p, RHS&& f) -> typename std::enable_if<
			true,
			Pipe_impl<RHS>>::type
		{
			auto wraped_func = std::forward_as_tuple(std::forward<RHS>(f));
			return std::move(*reinterpret_cast<Pipe_impl<RHS> *>(&wraped_func));
		}

			template <typename RHS, typename... FuncList>
		inline auto operator<<(Pipe_impl<FuncList...>&& p, RHS&& f) -> typename std::enable_if<
			true,
			Pipe_impl<FuncList..., RHS>>::type
		{
			auto funcs = std::tuple_cat(std::move(p.funcs), std::forward_as_tuple(std::forward<RHS>(f)));
			return std::move(*reinterpret_cast<Pipe_impl<FuncList..., RHS> *>(&funcs));
		}
	} // namespace impl

	template <typename... FuncList>
	inline auto pipe(FuncList &&...fl) -> typename std::enable_if<
		true,
		impl::Pipe_impl<FuncList...>>::type
	{
		return impl::Pipe_impl<FuncList...>{std::forward<FuncList>(fl)...};
	}

		inline impl::Pipe_impl<> pipe()
	{
		return impl::Pipe_impl<>{};
	}
} // namespace chaincall

#endif // _CHAINCALL_HPP
