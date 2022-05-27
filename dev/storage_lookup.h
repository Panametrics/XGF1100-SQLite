#pragma once

#include <type_traits>

#include "functional/cxx_polyfill.h"
#include "type_traits.h"

namespace sqlite_orm {
    namespace internal {

        template<class... Ts>
        struct storage_t;

        template<class... Ts>
        struct storage_impl;

        template<class T>
        struct is_storage : std::false_type {};

        template<class... Ts>
        struct is_storage<storage_t<Ts...>> : std::true_type {};
        template<class... Ts>
        struct is_storage<const storage_t<Ts...>> : std::true_type {};

        template<class T>
        struct is_storage_impl : std::false_type {};

        template<class... Ts>
        struct is_storage_impl<storage_impl<Ts...>> : std::true_type {};
        template<class... Ts>
        struct is_storage_impl<const storage_impl<Ts...>> : std::true_type {};

        /**
         *  std::true_type if given 'table' type matches, std::false_type otherwise.
         *  
         *  A 'table' type is one of: table_t<>, index_t<>
         */
        template<typename S, typename T>
        using table_type_matches = std::is_same<T, table_type_t<S>>;

        /**
         *  std::true_type if given object is mapped, std::false_type otherwise.
         * 
         *  Note: unlike table_t<>, index_t<>::object_type is always void.
         */
        template<typename S, typename O>
        using object_type_matches =
            typename polyfill::conjunction<polyfill::negation<std::is_void<storage_object_type_t<S>>>,
                                           std::is_same<O, storage_object_type_t<S>>>::type;

        /**
         *  std::true_type if given lookup type ('table' type, object) is mapped, std::false_type otherwise.
         * 
         *  Note: we allow lookup via S::table_type because it allows us to walk the storage_impl chain (in storage_impl_cat()).
         */
        template<typename S, typename Lookup>
        using lookup_type_matches =
            typename polyfill::disjunction<table_type_matches<S, Lookup>, object_type_matches<S, Lookup>>::type;
    }

    // pick/lookup metafunctions
    namespace internal {

        /**
         *  S - storage_impl type
         *  O - mapped data type
         */
        template<class S, class O, class SFINAE = void>
        struct storage_pick_impl_type;

        template<class S, class O>
        struct storage_pick_impl_type<S, O, match_if<lookup_type_matches, S, O>> {
            using type = S;
        };

        template<class S, class O>
        struct storage_pick_impl_type<S, O, match_if_not<lookup_type_matches, S, O>>
            : storage_pick_impl_type<typename S::super, O> {};

        /**
         *  Final specialisation
         */
        template<class O>
        struct storage_pick_impl_type<storage_impl<>, O, void> {};

        /**
         *  storage_t specialization
         */
        template<class... Ts, class O>
        struct storage_pick_impl_type<storage_t<Ts...>, O, void>
            : storage_pick_impl_type<typename storage_t<Ts...>::impl_type, O> {};

        /**
         *  S - storage_impl type
         *  O - mapped data type
         */
        template<class S, class O, class SFINAE = void>
        struct storage_find_impl_type;

        template<class S, class O>
        struct storage_find_impl_type<S, O, match_if<lookup_type_matches, S, O>> {
            using type = S;
        };

        template<class S, class O>
        struct storage_find_impl_type<S, O, match_if_not<lookup_type_matches, S, O>>
            : storage_find_impl_type<typename S::super, O> {};

        /**
         *  Final specialisation
         */
        template<class O>
        struct storage_find_impl_type<storage_impl<>, O, void> {
            using type = storage_impl<>;
        };

        /**
         *  storage_t specialization
         */
        template<class... Ts, class O>
        struct storage_find_impl_type<storage_t<Ts...>, O, void>
            : storage_find_impl_type<typename storage_t<Ts...>::impl_type, O> {};
    }

    namespace internal {

        template<typename T, typename U>
        using reapply_const_of_t = std::conditional_t<std::is_const<T>::value, std::add_const_t<U>, U>;

        /**
         *  S - storage_t or storage_impl type, possibly const-qualified
         *  Lookup - 'table' type, mapped data type
         */
        template<class S, class Lookup>
        using storage_pick_impl_t =
            reapply_const_of_t<S, type_t<storage_pick_impl_type<std::remove_const_t<S>, Lookup>>>;

        /**
         *  S - storage_t or storage_impl type, possibly const-qualified
         *  Lookup - 'table' type, mapped data type
         */
        template<class S, class Lookup>
        using storage_find_impl_t =
            reapply_const_of_t<S, type_t<storage_find_impl_type<std::remove_const_t<S>, Lookup>>>;
    }
}

// runtime lookup functions
namespace sqlite_orm {
    namespace internal {
        /**
         *  Given a storage implementation pack, pick the specific storage implementation for the given lookup type.
         * 
         *  Note: This function requires Lookup to be mapped, otherwise it is removed from the overload resolution set.
         */
        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        storage_pick_impl_t<S, Lookup>& pick_impl(S& impl) {
            return impl;
        }

        /**
         *  Given a storage implementation pack, find the specific storage implementation for the given lookup type.
         * 
         *  Note: This function returns the empty `storage_impl<>` if Lookup isn't mapped.
         */
        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        storage_find_impl_t<S, Lookup>& find_impl(S& impl) {
            return impl;
        }

        /**
         *  Given a storage, pick the specific const-qualified storage implementation for the lookup type.
         * 
         *  Note: This function requires Lookup to be mapped, otherwise it is removed from the overload resolution set.
         */
        template<class Lookup, class S, satisfies<is_storage, S> = true>
        storage_pick_impl_t<const S, Lookup>& pick_const_impl(S& storage) {
            return obtain_const_impl(storage);
        }
    }
}
