#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <mono/jit/jit.h>

//JS funcs
extern MonoObject* mono_wasm_invoke_js_with_args (int js_handle, MonoString *method, MonoArray *args, int *is_exception);
extern MonoObject* mono_wasm_get_object_property (int js_handle, MonoString *propertyName, int *is_exception);
extern MonoObject* mono_wasm_get_by_index (int js_handle, int property_index, int *is_exception);
extern MonoObject* mono_wasm_set_object_property (int js_handle, MonoString *propertyName, MonoObject *value, int createIfNotExist, int hasOwnProperty, int *is_exception);
extern MonoObject* mono_wasm_set_by_index (int js_handle, int property_index, MonoObject *value, int *is_exception);
extern MonoObject* mono_wasm_get_global_object (MonoString *global_name, int *is_exception);
extern void* mono_wasm_release_handle (int js_handle, int *is_exception);
extern void* mono_wasm_release_object (int js_handle, int *is_exception);
extern MonoObject* mono_wasm_new_object (int js_handle, MonoArray *args, int *is_exception);
extern MonoObject* mono_wasm_new (MonoString *core_name, MonoArray *args, int *is_exception);
extern int mono_wasm_bind_core_object (int js_handle, int gc_handle, int *is_exception);
extern int mono_wasm_bind_host_object (int js_handle, int gc_handle, int *is_exception);
extern MonoObject* mono_wasm_typed_array_to_array (int js_handle, int *is_exception);
extern MonoObject* mono_wasm_typed_array_copy_to (int js_handle, int ptr, int begin, int end, int bytes_per_element, int *is_exception);
extern MonoObject* mono_wasm_typed_array_from (int ptr, int begin, int end, int bytes_per_element, int type, int *is_exception);
extern MonoObject* mono_wasm_typed_array_copy_from (int js_handle, int ptr, int begin, int end, int bytes_per_element, int *is_exception);

void core_initialize_internals ()
{
	mono_add_internal_call ("WebAssembly.Runtime::InvokeJSWithArgs", mono_wasm_invoke_js_with_args);
	mono_add_internal_call ("WebAssembly.Runtime::GetObjectProperty", mono_wasm_get_object_property);
	mono_add_internal_call ("WebAssembly.Runtime::GetByIndex", mono_wasm_get_by_index);
	mono_add_internal_call ("WebAssembly.Runtime::SetObjectProperty", mono_wasm_set_object_property);
	mono_add_internal_call ("WebAssembly.Runtime::SetByIndex", mono_wasm_set_by_index);
	mono_add_internal_call ("WebAssembly.Runtime::GetGlobalObject", mono_wasm_get_global_object);
	mono_add_internal_call ("WebAssembly.Runtime::ReleaseHandle", mono_wasm_release_handle);
	mono_add_internal_call ("WebAssembly.Runtime::ReleaseObject", mono_wasm_release_object);
	mono_add_internal_call ("WebAssembly.Runtime::NewObjectJS", mono_wasm_new_object);
	mono_add_internal_call ("WebAssembly.Runtime::BindCoreObject", mono_wasm_bind_core_object);
	mono_add_internal_call ("WebAssembly.Runtime::BindHostObject", mono_wasm_bind_host_object);
	mono_add_internal_call ("WebAssembly.Runtime::New", mono_wasm_new);
	mono_add_internal_call ("WebAssembly.Runtime::TypedArrayToArray", mono_wasm_typed_array_to_array);
	mono_add_internal_call ("WebAssembly.Runtime::TypedArrayCopyTo", mono_wasm_typed_array_copy_to);
	mono_add_internal_call ("WebAssembly.Runtime::TypedArrayFrom", mono_wasm_typed_array_from);
	mono_add_internal_call ("WebAssembly.Runtime::TypedArrayCopyFrom", mono_wasm_typed_array_copy_from);

}

// Int8Array 		| int8_t	| byte or SByte (signed byte)
// Uint8Array		| uint8_t	| byte or Byte (unsigned byte)
// Uint8ClampedArray| uint8_t	| byte or Byte (unsigned byte)
// Int16Array		| int16_t	| short (signed short)
// Uint16Array		| uint16_t	| ushort (unsigned short)
// Int32Array		| int32_t	| int (signed integer)
// Uint32Array		| uint32_t	| uint (unsigned integer)
// Float32Array		| float		| float
// Float64Array		| double	| double
// typed array marshalling
#define MARSHAL_ARRAY_BYTE 11
#define MARSHAL_ARRAY_UBYTE 12
#define MARSHAL_ARRAY_SHORT 13
#define MARSHAL_ARRAY_USHORT 14
#define MARSHAL_ARRAY_INT 15
#define MARSHAL_ARRAY_UINT 16
#define MARSHAL_ARRAY_FLOAT 17
#define MARSHAL_ARRAY_DOUBLE 18

EMSCRIPTEN_KEEPALIVE MonoArray*
mono_wasm_typed_array_new (char *arr, int length, int size, int type)
{
	MonoClass *typeClass = mono_get_byte_class(); // default is Byte
	switch (type) {
	case MARSHAL_ARRAY_BYTE:
		typeClass = mono_get_sbyte_class();
		break;
	case MARSHAL_ARRAY_SHORT:
		typeClass = mono_get_int16_class();
		break;
	case MARSHAL_ARRAY_USHORT:
		typeClass = mono_get_uint16_class();
		break;
	case MARSHAL_ARRAY_INT:
		typeClass = mono_get_int32_class();
		break;
	case MARSHAL_ARRAY_UINT:
		typeClass = mono_get_uint32_class();
		break;
	case MARSHAL_ARRAY_FLOAT:
		typeClass = mono_get_single_class();
		break;
	case MARSHAL_ARRAY_DOUBLE:
		typeClass = mono_get_double_class();
		break;
	}

	MonoArray *buffer;

	buffer = mono_array_new (mono_get_root_domain(), typeClass, length);
	memcpy(mono_array_addr_with_size(buffer, sizeof(char), 0), arr, length * size);

	return buffer;
}

EMSCRIPTEN_KEEPALIVE int
mono_wasm_unbox_enum (MonoObject *obj)
{
	if (!obj)
		return 0;
	
	MonoType *type = mono_class_get_type (mono_object_get_class(obj));

	void *ptr = mono_object_unbox (obj);
	switch (mono_type_get_type(mono_type_get_underlying_type (type))) {
	case MONO_TYPE_I1:
	case MONO_TYPE_U1:
		return *(unsigned char*)ptr;
	case MONO_TYPE_I2:
		return *(short*)ptr;
	case MONO_TYPE_U2:
		return *(unsigned short*)ptr;
	case MONO_TYPE_I4:
		return *(int*)ptr;
	case MONO_TYPE_U4:
		return *(unsigned int*)ptr;
	// WASM doesn't support returning longs to JS
	// case MONO_TYPE_I8:
	// case MONO_TYPE_U8:
	default:
		printf ("Invalid type %d to mono_unbox_enum\n", mono_type_get_type(mono_type_get_underlying_type (type)));
		return 0;
	}
}


