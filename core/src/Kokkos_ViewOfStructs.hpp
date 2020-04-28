
#ifndef KOKKOS_VIEWOFSTRUCTS_HPP
#define KOKKOS_VIEWOFSTRUCTS_HPP

#include <type_traits>

namespace Kokkos {



/*
 * A wrapper type to pass around integers for field indices at compile time
 */
template<size_t index>
struct Field {
};


/*
 * A wrapper type to represent a struct being stored
 */
template<class FirstType, class... RestTypes>
struct Struct : Struct<RestTypes...> {

  static constexpr size_t number_fields = 1 + Struct<RestTypes...>::number_fields;

  // The type of the indexed field
  template<size_t field_index>
  using field_type = typename std::conditional<
                                field_index == 0,
                                FirstType,
                                typename Struct<RestTypes...>::template field_type<field_index-1>
                              >::type;

  // The type of the index field as an lvalue reference
  template<size_t field_index>
  using field_reference_type = typename std::add_lvalue_reference<field_type<field_index>>::type;


  FirstType field_content;

  
  Struct()
    : field_content(), Struct<RestTypes...>() {
  }

  Struct(FirstType arg, RestTypes... rest)
    : field_content(arg), Struct<RestTypes...>(rest...) {
  }

  
  template<size_t field_index>
  field_reference_type<field_index> operator()(Field<field_index>) {
    return Struct<RestTypes...>::operator()(Field<field_index-1>());
  }

  field_reference_type<0> operator()(Field<0>) {
    return field_content;
  }
  
};

// Base case for structs
template<class LastType>
struct Struct<LastType> {

  static constexpr size_t number_fields = 1;
  
  template<size_t field_index>
  using field_type = typename std::conditional<field_index == 0, LastType, void>::type;

  template<size_t field_index>
  using field_reference_type = typename std::add_lvalue_reference<field_type<field_index>>::type;


  LastType field_content;

  Struct()
    : field_content() {
  }

  Struct(LastType arg)
    : field_content(arg) {
  }

  template<size_t field_index>
  field_reference_type<field_index> operator()(Field<field_index>) {
    static_assert(field_index == 0, "Invalid field access");

    return field_content;
  }

};


namespace Impl {

template<class Old, class New>
struct copy_array_qualifiers {
  typedef New type;
};

template<class Old, class New>
struct copy_array_qualifiers<Old*, New> {
  typedef typename std::add_pointer<typename copy_array_qualifiers<Old, New>::type>::type
          type;
};

template<class Old, class New, size_t N>
struct copy_array_qualifiers<Old[N], New> {
  typedef typename copy_array_qualifiers<Old, New>::type
          arrayless_type;
  typedef arrayless_type
          type[N];
};


template<class ViewTraits, class = typename ViewTraits::array_layout>
class ViewOfStructsStorage {
  static_assert(std::is_same<typename ViewTraits::array_layout, LayoutRight>::value
                || std::is_same<typename ViewTraits::array_layout, LayoutLeft>::value,
                "Unsupported Layout");
};

// internal storage for View of Structs layout
template<class ViewTraits>
class ViewOfStructsStorage<ViewTraits, LayoutRight> {

  using storage_type = View<typename ViewTraits::data_type,
                            typename ViewTraits::array_layout,
                            typename ViewTraits::memory_space,
                            typename ViewTraits::memory_traits>;
          
  storage_type storage;

public:

  template<class... Args>
  ViewOfStructsStorage<ViewTraits, LayoutRight>(Args&&... args)
    : storage(args...) {
  }

  template <typename I0, size_t field_index>
  KOKKOS_FORCEINLINE_FUNCTION
      typename ViewTraits::value_type::template field_reference_type<field_index>
      operator()(const I0& i0, const Field<field_index>& field) const {
    return storage(i0)(field);
  }
};
 

// internal storage for Struct of Views layout
template<class ViewTraits>
class ViewOfStructsStorage<ViewTraits, LayoutLeft> {


  // internal type to hold the View for each field
  template<class StructType, size_t field_index = 0, class Enable = void>
  struct Storage : Storage<StructType, field_index+1> {

    typedef typename StructType::template field_type<field_index>
            field_type;
    typedef typename copy_array_qualifiers<typename ViewTraits::data_type, field_type>::type
            qualified_field_type;

    View<qualified_field_type> field_storage;

    template<class... Args>
    Storage(Args&&... args)
      : field_storage(args...),
        Storage<StructType, field_index+1>(args...) {
    }

    template <typename I0, size_t accessed_field_index>
    KOKKOS_FORCEINLINE_FUNCTION
        typename ViewTraits::value_type::template field_reference_type<accessed_field_index>
        operator()(const I0 i0, const Field<accessed_field_index> field) const {

      return Storage<StructType, field_index+1>::operator()(i0, field);
    }
    template <typename I0>
    KOKKOS_FORCEINLINE_FUNCTION
        typename ViewTraits::value_type::template field_reference_type<field_index>
        operator()(const I0 i0, const Field<field_index> field) const {

      return field_storage(i0);
    }
  };

  


  template<class StructType, size_t field_index>
  struct Storage<StructType,
                 field_index,
                 typename std::enable_if<field_index == StructType::number_fields>::type> {

    template<class... Args>
    Storage(Args&&... args) {
    }

    template <typename I0, size_t accessed_field_index>
    KOKKOS_FORCEINLINE_FUNCTION
        typename ViewTraits::value_type::template field_reference_type<accessed_field_index>
        operator()(const I0& i0, const Field<accessed_field_index>) const {
      assert("Invalid field index");
    }
  };

  Storage<typename ViewTraits::value_type> storage;

public:

  template<class... Args>
  ViewOfStructsStorage(Args&&... args) : storage(args...) {
  }

  template <typename I0, size_t field_index>
  KOKKOS_FORCEINLINE_FUNCTION
      typename ViewTraits::value_type::template field_reference_type<field_index>
      operator()(const I0& i0, const Field<field_index>& field) const {
    return storage(i0, field);
  }

};

} // namespace Impl


template <class DataType, class... Properties>
class ViewOfStructs {

  using view_traits = ViewTraits<DataType, Properties...>;

  using struct_type = typename view_traits::value_type;

  Impl::ViewOfStructsStorage<view_traits> storage;

public:

  template<class... Args>
  ViewOfStructs(Args&&... args) : storage(args...) {
  }

  // Rank 1 accessor
  template <typename I0, size_t field_index>
      // TODO add inline/enable_if code
      typename struct_type::template field_reference_type<field_index>
      operator()(const I0& i0, const Field<field_index>& field) const {
    return storage(i0, field);
  }

};

} // namespace Kokkos


#endif // KOKKOS_VIEWSOFSTRUCTS_HPP
