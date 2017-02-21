#pragma once

template <typename Func, typename... Args>
decltype(auto) CallComFunc( Func&& function, Args&&... args )
{
	return CallComFuncIntenal( std::is_member_function_pointer<Func>( ), std::forward<Func>( function ), std::forward<Args>( args )... );
}

template <typename Func, typename... Args>
decltype(auto) CallComFuncIntenal( std::false_type, Func function, Args&&... args )
{
	return (static_cast<HRESULT>(function( std::forward<Args>( args )... )) >= 0);
}

template <typename Func, typename ClassType, typename... Args>
decltype(auto) CallComFuncIntenal( std::true_type, Func function, ClassType&& self, Args&&... args )
{
	HRESULT hr = static_cast<HRESULT>(((*self.Get( )).*function)(std::forward<Args>( args )...));
	return ( hr >= 0L );
}