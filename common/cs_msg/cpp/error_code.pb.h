// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: error_code.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_error_5fcode_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_error_5fcode_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3010000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3010000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_error_5fcode_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_error_5fcode_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxillaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_error_5fcode_2eproto;
PROTOBUF_NAMESPACE_OPEN
PROTOBUF_NAMESPACE_CLOSE
namespace net {

enum RESULT_CODE : int {
  RESULT_CODE_FAIL = 0,
  RESULT_CODE_SUCCESS = 1,
  RESULT_CODE_CION_ERROR = 3,
  RESULT_CODE_PASSWD_ERROR = 4,
  RESULT_CODE_NEED_INLOBBY = 5,
  RESULT_CODE_REPEAT_GET = 6,
  RESULT_CODE_NOT_COND = 7,
  RESULT_CODE_ERROR_PARAM = 8,
  RESULT_CODE_NOT_TABLE = 9,
  RESULT_CODE_NOT_OWER = 10,
  RESULT_CODE_BLACKLIST = 11,
  RESULT_CODE_NOT_DIAMOND = 12,
  RESULT_CODE_ERROR_PLAYERID = 13,
  RESULT_CODE_TABLE_FULL = 14,
  RESULT_CODE_GAMEING = 15,
  RESULT_CODE_ERROR_STATE = 16,
  RESULT_CODE_LOGIN_OTHER = 17,
  RESULT_CODE_SVR_REPAIR = 18,
  RESULT_CODE_CDING = 19,
  RESULT_CODE_EXIST_OBJ = 20,
  RESULT_CODE_ENTER_SVR_FAIL = 21,
  RESULT_CODE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  RESULT_CODE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool RESULT_CODE_IsValid(int value);
constexpr RESULT_CODE RESULT_CODE_MIN = RESULT_CODE_FAIL;
constexpr RESULT_CODE RESULT_CODE_MAX = RESULT_CODE_ENTER_SVR_FAIL;
constexpr int RESULT_CODE_ARRAYSIZE = RESULT_CODE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* RESULT_CODE_descriptor();
template<typename T>
inline const std::string& RESULT_CODE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, RESULT_CODE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function RESULT_CODE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    RESULT_CODE_descriptor(), enum_t_value);
}
inline bool RESULT_CODE_Parse(
    const std::string& name, RESULT_CODE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<RESULT_CODE>(
    RESULT_CODE_descriptor(), name, value);
}
enum MISSION_TYPE : int {
  MISSION_TYPE_NULL = 0,
  MISSION_TYPE_PLAY = 1,
  MISSION_TYPE_WIN = 2,
  MISSION_TYPE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  MISSION_TYPE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool MISSION_TYPE_IsValid(int value);
constexpr MISSION_TYPE MISSION_TYPE_MIN = MISSION_TYPE_NULL;
constexpr MISSION_TYPE MISSION_TYPE_MAX = MISSION_TYPE_WIN;
constexpr int MISSION_TYPE_ARRAYSIZE = MISSION_TYPE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* MISSION_TYPE_descriptor();
template<typename T>
inline const std::string& MISSION_TYPE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, MISSION_TYPE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function MISSION_TYPE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    MISSION_TYPE_descriptor(), enum_t_value);
}
inline bool MISSION_TYPE_Parse(
    const std::string& name, MISSION_TYPE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<MISSION_TYPE>(
    MISSION_TYPE_descriptor(), name, value);
}
enum MISSION_CYCLE_TYPE : int {
  MISSION_CYCLE_TYPE_DAY = 0,
  MISSION_CYCLE_TYPE_WEEK = 1,
  MISSION_CYCLE_TYPE_MONTH = 2,
  MISSION_CYCLE_TYPE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  MISSION_CYCLE_TYPE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool MISSION_CYCLE_TYPE_IsValid(int value);
constexpr MISSION_CYCLE_TYPE MISSION_CYCLE_TYPE_MIN = MISSION_CYCLE_TYPE_DAY;
constexpr MISSION_CYCLE_TYPE MISSION_CYCLE_TYPE_MAX = MISSION_CYCLE_TYPE_MONTH;
constexpr int MISSION_CYCLE_TYPE_ARRAYSIZE = MISSION_CYCLE_TYPE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* MISSION_CYCLE_TYPE_descriptor();
template<typename T>
inline const std::string& MISSION_CYCLE_TYPE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, MISSION_CYCLE_TYPE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function MISSION_CYCLE_TYPE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    MISSION_CYCLE_TYPE_descriptor(), enum_t_value);
}
inline bool MISSION_CYCLE_TYPE_Parse(
    const std::string& name, MISSION_CYCLE_TYPE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<MISSION_CYCLE_TYPE>(
    MISSION_CYCLE_TYPE_descriptor(), name, value);
}
enum GAME_CATE_TYPE : int {
  GAME_CATE_NULL = 0,
  GAME_CATE_LAND = 1,
  GAME_CATE_KUAIPAO = 2,
  GAME_CATE_MAX_TYPE = 3,
  GAME_CATE_TYPE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  GAME_CATE_TYPE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool GAME_CATE_TYPE_IsValid(int value);
constexpr GAME_CATE_TYPE GAME_CATE_TYPE_MIN = GAME_CATE_NULL;
constexpr GAME_CATE_TYPE GAME_CATE_TYPE_MAX = GAME_CATE_MAX_TYPE;
constexpr int GAME_CATE_TYPE_ARRAYSIZE = GAME_CATE_TYPE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* GAME_CATE_TYPE_descriptor();
template<typename T>
inline const std::string& GAME_CATE_TYPE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, GAME_CATE_TYPE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function GAME_CATE_TYPE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    GAME_CATE_TYPE_descriptor(), enum_t_value);
}
inline bool GAME_CATE_TYPE_Parse(
    const std::string& name, GAME_CATE_TYPE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<GAME_CATE_TYPE>(
    GAME_CATE_TYPE_descriptor(), name, value);
}
enum TABLE_FEE_TYPE : int {
  TABLE_FEE_TYPE_NO = 0,
  TABLE_FEE_TYPE_ALLBASE = 1,
  TABLE_FEE_TYPE_WIN = 2,
  TABLE_FEE_TYPE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  TABLE_FEE_TYPE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool TABLE_FEE_TYPE_IsValid(int value);
constexpr TABLE_FEE_TYPE TABLE_FEE_TYPE_MIN = TABLE_FEE_TYPE_NO;
constexpr TABLE_FEE_TYPE TABLE_FEE_TYPE_MAX = TABLE_FEE_TYPE_WIN;
constexpr int TABLE_FEE_TYPE_ARRAYSIZE = TABLE_FEE_TYPE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* TABLE_FEE_TYPE_descriptor();
template<typename T>
inline const std::string& TABLE_FEE_TYPE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, TABLE_FEE_TYPE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function TABLE_FEE_TYPE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    TABLE_FEE_TYPE_descriptor(), enum_t_value);
}
inline bool TABLE_FEE_TYPE_Parse(
    const std::string& name, TABLE_FEE_TYPE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<TABLE_FEE_TYPE>(
    TABLE_FEE_TYPE_descriptor(), name, value);
}
enum TABLE_STATE : int {
  TABLE_STATE_FREE = 0,
  TABLE_STATE_CALL = 1,
  TABLE_STATE_PLAY = 2,
  TABLE_STATE_WAIT = 3,
  TABLE_STATE_GAME_END = 4,
  TABLE_STATE_PIAO_FEN = 5,
  TABLE_STATE_RECYCLE = 6,
  TABLE_STATE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  TABLE_STATE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool TABLE_STATE_IsValid(int value);
constexpr TABLE_STATE TABLE_STATE_MIN = TABLE_STATE_FREE;
constexpr TABLE_STATE TABLE_STATE_MAX = TABLE_STATE_RECYCLE;
constexpr int TABLE_STATE_ARRAYSIZE = TABLE_STATE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* TABLE_STATE_descriptor();
template<typename T>
inline const std::string& TABLE_STATE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, TABLE_STATE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function TABLE_STATE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    TABLE_STATE_descriptor(), enum_t_value);
}
inline bool TABLE_STATE_Parse(
    const std::string& name, TABLE_STATE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<TABLE_STATE>(
    TABLE_STATE_descriptor(), name, value);
}
enum emACCTRAN_OPER_TYPE : int {
  emACCTRAN_OPER_TYPE_NULL = 0,
  emACCTRAN_OPER_TYPE_BuyCard = 1,
  emACCTRAN_OPER_TYPE_GIVE = 2,
  emACCTRAN_OPER_TYPE_GAME = 3,
  emACCTRAN_OPER_TYPE_TASK = 4,
  emACCTRAN_OPER_TYPE_FEE = 5,
  emACCTRAN_OPER_TYPE_LOGIN = 6,
  emACCTRAN_OPER_TYPE_BANKRUPT = 7,
  emACCTRAN_OPER_TYPE_MAIL = 8,
  emACCTRAN_OPER_TYPE_SAFEBOX = 9,
  emACCTRAN_OPER_TYPE_PROOM = 10,
  emACCTRAN_OPER_TYPE_BACKSITE = 11,
  emACCTRAN_OPER_TYPE_SysGive = 12,
  emACCTRAN_OPER_TYPE_SysChange = 13,
  emACCTRAN_OPER_TYPE_Exchange = 14,
  emACCTRAN_OPER_TYPE_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  emACCTRAN_OPER_TYPE_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool emACCTRAN_OPER_TYPE_IsValid(int value);
constexpr emACCTRAN_OPER_TYPE emACCTRAN_OPER_TYPE_MIN = emACCTRAN_OPER_TYPE_NULL;
constexpr emACCTRAN_OPER_TYPE emACCTRAN_OPER_TYPE_MAX = emACCTRAN_OPER_TYPE_Exchange;
constexpr int emACCTRAN_OPER_TYPE_ARRAYSIZE = emACCTRAN_OPER_TYPE_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* emACCTRAN_OPER_TYPE_descriptor();
template<typename T>
inline const std::string& emACCTRAN_OPER_TYPE_Name(T enum_t_value) {
  static_assert(::std::is_same<T, emACCTRAN_OPER_TYPE>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function emACCTRAN_OPER_TYPE_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    emACCTRAN_OPER_TYPE_descriptor(), enum_t_value);
}
inline bool emACCTRAN_OPER_TYPE_Parse(
    const std::string& name, emACCTRAN_OPER_TYPE* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<emACCTRAN_OPER_TYPE>(
    emACCTRAN_OPER_TYPE_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace net

PROTOBUF_NAMESPACE_OPEN

template <> struct is_proto_enum< ::net::RESULT_CODE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::RESULT_CODE>() {
  return ::net::RESULT_CODE_descriptor();
}
template <> struct is_proto_enum< ::net::MISSION_TYPE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::MISSION_TYPE>() {
  return ::net::MISSION_TYPE_descriptor();
}
template <> struct is_proto_enum< ::net::MISSION_CYCLE_TYPE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::MISSION_CYCLE_TYPE>() {
  return ::net::MISSION_CYCLE_TYPE_descriptor();
}
template <> struct is_proto_enum< ::net::GAME_CATE_TYPE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::GAME_CATE_TYPE>() {
  return ::net::GAME_CATE_TYPE_descriptor();
}
template <> struct is_proto_enum< ::net::TABLE_FEE_TYPE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_FEE_TYPE>() {
  return ::net::TABLE_FEE_TYPE_descriptor();
}
template <> struct is_proto_enum< ::net::TABLE_STATE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::TABLE_STATE>() {
  return ::net::TABLE_STATE_descriptor();
}
template <> struct is_proto_enum< ::net::emACCTRAN_OPER_TYPE> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::net::emACCTRAN_OPER_TYPE>() {
  return ::net::emACCTRAN_OPER_TYPE_descriptor();
}

PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_error_5fcode_2eproto
