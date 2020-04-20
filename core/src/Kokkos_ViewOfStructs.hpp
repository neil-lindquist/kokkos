
#ifndef KOKKOS_VIEWOFSTRUCTS_HPP
#define KOKKOS_VIEWOFSTRUCTS_HPP

#include <type_traits>

namespace Kokkos {

/*
 * A wrapper type to represent a struct being stored
 */
template<class FirstType, class... RestTypes>
struct Struct {

  static constexpr size_t number_fields = 1 + Struct<RestTypes...>::number_fields;

  // The type of the indexed field
  template<size_t field_index>
  using field_type = typename std::conditional<
                                field_index == 0,
                                FirstType,
                                typename Struct<RestTypes...>::template filed_type<field_index-1>
                              >::type;

  // The type of the index field as an lvalue reference
  template<size_t field_index>
  using field_reference_type = typename std::add_lvalue_reference<field_type<field_index>>::type;
};

// Base case for structs
template<class LastType>
struct Struct<LastType> {

  static constexpr size_t number_fields = 1;
  
  template<size_t field_index>
  using field_type = typename std::enable_if<field_index == 0, LastType>::type;

  template<size_t field_index>
  using field_reference_type = typename std::add_lvalue_reference<field_type<field_index>>::type;
};

/*
 * A wrapper type to pass around integers for field indices at compile time
 */
template<size_t index>
struct Field {
};

namespace Impl {

template<class StructType, size_t field_index = 0>
struct StructAlignedLength {
  static constexpr size_t value = alignof(typename StructType::template field_type<field_index>)
                                  + field_index + 1 < StructType::number_fields
                                    ? StructAlignedLength<StructType, field_index+1>::value
                                    : 0;
};


} // namespace Impl


template <class DataType, class... Properties>
class ViewOfStructs {

  // TODO typedef the type w/out pointers or array lengths as StructType

public:

  // Rank 1 accessor
  template <typename I0, size_t field_index>
      // TODO add inline/enable_if code
      typename StructType::field_reference_type<field_index>
      operator()(const I0& i0, const Field<field_index>) const {
    // TODO implement
  }

};

} // namespace Kokkos


#endif // KOKKOS_VIEWSOFSTRUCTS_HPP
