/*
 * Simple Dalvik Virtual Machine Implementation
 *
 * Copyright (C) 2013 Chun-Yu Wang <wicanr2@gmail.com>
 */

#ifndef SIMPLE_DVM_H
#define SIMPLE_DVM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef signed char s1;
typedef short u2;
typedef signed short s2;
typedef long long int s8;

typedef unsigned int u4;
typedef unsigned int uint;
typedef unsigned char u1;
typedef unsigned short ushort;
typedef unsigned char byte;
typedef unsigned long long u8;

#define TRUE 1
#define FALSE 0

/*  I only add my needed define, more in :
 *  http://source.android.com/devices/tech/dalvik/dex-format.html
 */
#define ACC_PUBLIC      0x1
#define ACC_PRIVATE     0x2
#define ACC_PROTECTED   0x4
#define ACC_STATIC      0x8
#define ACC_FINAL       0x10
#define ACC_CONSTRUCTOR 0x10000

extern const uint NO_INDEX;

/* map_list */
typedef struct _map_item {
    ushort  type;
    ushort  unused;
    uint    size;
    uint    offset;
} map_item;
typedef struct _map_list {
    uint size ;
    map_item *map_item;
} map_list;

typedef struct _type_item {
    ushort type_idx;
} type_item;
typedef struct _type_list {
    uint size ;
    type_item *type_item;
} type_list;

/* string_ids */
typedef struct _string_data_item {
    int index;
    int uleb128_len;
    u1  data[255];
} string_data_item;

typedef struct _string_ids {
    u4  string_data_off;
} string_ids;

/* type_ids */
typedef struct _type_id_item {
    uint descriptor_idx;
} type_id_item;

/* proto_ids */
typedef struct _proto_id_item {
    uint shorty_idx;      /* pointer to string_data_item */
    uint return_type_idx; /* pointer to type_ids */
    uint parameters_off;
} proto_id_item;

/* field_ids */
typedef struct _field_id_item {
    ushort class_idx; /* index into the type_ids list for the definer of this field */
    ushort type_idx;
    uint   name_idx;
} field_id_item;

/* method_ids */
typedef struct _method_id_tiem {
    ushort  class_idx; /* index into the type_ids list for the definer of this method */
    ushort  proto_idx; /* index into the proto_ids list for the prototype of this method */
    uint    name_idx;
} method_id_item;

/* class defs */
typedef struct _code_item {
    ushort registers_size;
    ushort ins_size;
    ushort outs_size;
    ushort tries_size;
    uint   debug_info_off;
    uint   insns_size;
    ushort *insns;
    /*
    ushort padding;
    try_item
    handlers
    */
} code_item;

typedef struct _encoded_method {
    uint method_id;
    uint access_flags;
    uint code_off;
    code_item code_item;
} encoded_method;

typedef struct _encoded_field {
    /*  https://ntu-android-2014.hackpad.com/Notes-and-QAs-for-Homework-3-SZVgjTYruKX
        http://source.android.com/devices/tech/dalvik/dex-format.html
        LEB128 :
        . a variable-length encoding for arbitrary signed or unsigned integer quantities
        . DWARF3
        . In a .dex file, LEB128 is only ever used to encode 32-bit quantities
          => 1 ~ 5 bytes
          => So, we can safely use 'uint'
     */
    uint field_id;  /* Note! we store id, not index diff! */
    uint access_flags;
    uint field_size;    /* Note! object size is always sizeof(sdvm_obj) */
    /*  this_ptr + offset = the field starting address in object instance
     *  ref : comment in sdvm_obj */
    uint offset;
} encoded_field;

typedef enum _static_data_value_type {
    VALUE_BYTE = 0x00,
    VALUE_SHORT = 0x02,
    VALUE_CHAR = 0x03,
    VALUE_INT = 0x04,
    VALUE_LONG = 0x06,
    VALUE_FLOAT = 0x10,
    VALUE_DOUBLE = 0x11,
    VALUE_STRING = 0x17,
    VALUE_TYPE = 0x18,
    VALUE_FIELD = 0x19,
    VALUE_METHOD = 0x1a,
    VALUE_ENUM = 0x1b,
    VALUE_ARRAY = 0x1c,
    VALUE_ANNOTATION = 0x1d,
    VALUE_NULL = 0x1e,
    VALUE_BOOLEAN = 0x1f,
    VALUE_SDVM_OBJ = 0x20   /* sdvm defined, means this is a sdvm object */
} static_data_value_type;

/*  sdvm object layout define (merely same as c++, except we don't handle vtbl
 *  currently) :
 *
 *    low address
 *  +-------------+  <-- this ptr
 *  | sdvm obj    |
 *  +-------------+
 *  | super class |
 *  | members     |
 *  +-------------+
 *  | base class  |
 *  | members     |
 *  +-------------+
 *
 *  * Assume we at most have 1 super class
 */
typedef struct _sdvm_obj {
    uint ref_count;
    struct _class_data_item *clazz;
    void *other_data;
} sdvm_obj;

typedef struct _static_field_data {
    /*  e.g. Msg.out = System.err;
     *  out is a static object, err is an object, out will actually point to err
     *  (My understanding)
     */
    sdvm_obj    *obj;
    static_data_value_type type;
    //static_field_data *child;
    //uint field_id;
} static_field_data;

typedef struct _class_def_item {
    uint class_idx;
    uint access_flags;
    uint superclass_idx; /* index into typeIds for superclass */
    uint interfaces_off; /* file offset to DexTypeList */
    uint source_file_idx; /* index into stringIds for source file name */
    uint annotations_off; /* file offset to annotations_directory_item */
    uint class_data_off; /* file offset to class_data_item */
    uint static_values_off; /* file offset to DexEncodedArray */
} class_def_item;

typedef struct _class_data_item {
    uint static_fields_size;
    uint instance_fields_size;
    uint direct_methods_size;
    uint virtual_methods_size;

    /* ref : https://ntu-android-2014.hackpad.com/Notes-and-QAs-for-Homework-3-SZVgjTYruKX */
    encoded_field *static_fields;
    encoded_field *instance_fields;
    encoded_method *direct_methods;
    encoded_method *virtual_methods;

    struct _class_def_item *clazz_def;
    struct _class_data_item *super_class;
    static_field_data *sdata;
    //uint field_id_start;
    //uint field_id_count;

    /*  how many bytes I need to allocate if I instantiate it
     *  (include parent class member)
     *
     *  instance_fields tell me the object layout
     */
    uint class_inst_size;
} class_data_item;

typedef struct _DexHeader {
    u1 magic[8]; /* includes version number */
    u1 checksum[4]; /* adler32 checksum */
    u1 signature[20]; /* SHA-1 hash */
    u4 fileSize; /* length of entire file */
    u4 headerSize; /* offset to start of next section */
    u4 endianTag;
    u4 linkSize;
    u4 linkOff;
    u4 mapOff;
    u4 stringIdsSize;
    u4 stringIdsOff;
    u4 typeIdsSize;
    u4 typeIdsOff;
    u4 protoIdsSize;
    u4 protoIdsOff;
    u4 fieldIdsSize;
    u4 fieldIdsOff;
    u4 methodIdsSize;
    u4 methodIdsOff;
    u4 classDefsSize;
    u4 classDefsOff;
    u4 dataSize;
    u4 dataOff;
} DexHeader;

typedef struct DexFileFormat {
    DexHeader header;
    string_ids       *string_ids;
    string_data_item *string_data_item;
    type_id_item     *type_id_item;
    proto_id_item    *proto_id_item;
    type_list        *proto_type_list;
    field_id_item    *field_id_item;
    method_id_item   *method_id_item;
    class_def_item   *class_def_item;
    class_data_item  *class_data_item;

    static_field_data*undef_sdata;  /* for those undefined static obj */
    uint             undef_id_start;
    //class_def_item   *undef_class_def_item;
    //class_data_item  *undef_class_data_item;

    map_list         map_list;
    type_list        type_list;
    u1               *data;
} DexFileFormat;

/* Dex File Parser */
int parseDexFile(char *file, DexFileFormat *dex);
void printDexFile(DexFileFormat *dex);

/* map list parser */
void parse_map_list(DexFileFormat *dex, unsigned char *buf, int offset);

/* String ids parser */
void parse_string_ids(DexFileFormat *dex, unsigned char *buf, int offset);
char *get_string_data(DexFileFormat *dex, int string_id);

/* type_ids parser */
void parse_type_ids(DexFileFormat *dex, unsigned char *buf, int offset);
type_id_item *get_type_item(DexFileFormat *dex, int type_id);
char *get_type_item_name(DexFileFormat *dex, int type_id);

/* proto_ids parser */
void parse_proto_ids(DexFileFormat *dex, unsigned char *buf, int offset);
proto_id_item *get_proto_item(DexFileFormat *dex, int proto_id);
type_list *get_proto_type_list(DexFileFormat *dex, int proto_id);

/* field_ids parser */
void parse_field_ids(DexFileFormat *dex, unsigned char *buf, int offset);
field_id_item *get_field_item(DexFileFormat *dex, int field_id);
const char *get_field_name(DexFileFormat *dex, int field_id);
const char *get_class_name(DexFileFormat *dex, int field_id);

/* method ids parser */
void parse_method_ids(DexFileFormat *dex, unsigned char *buf, int offset);
method_id_item *get_method_item(DexFileFormat *dex, int method_id);

/* class defs parser */
void parse_class_defs(DexFileFormat *dex, unsigned char *buf, int offset);
sdvm_obj * get_static_obj_by_fieldid(DexFileFormat *dex, const int fieldid);
static_field_data * get_static_field_data_by_fieldid(DexFileFormat *dex, const int fieldid);
class_data_item *get_class_data_by_typeid(DexFileFormat *dex, const int type_id);
class_data_item *get_class_data_by_typeid_in_range(DexFileFormat *dex, const int type_id,
                                                   const uint max_count);
int get_uleb128_len(unsigned char *buf, int offset, int *size);

/* generic parameter parser for 35c */
typedef struct _invoke_parameters {
    int method_id;
    int reg_count;
    int reg_idx[5]; // 0-5 map C-G
} invoke_parameters;

/* Dalvik VM Register Bank */
typedef struct _simple_dvm_register {
    u1 data[4];
} simple_dvm_register;

typedef struct _simple_dalvik_vm {
    u1 heap[8192];
    //u4 stack[8192];
    u2 stack_ptr;
    u1 object_ref[4];
    //sdvm_obj *object_ref;
    simple_dvm_register regs[32];
    invoke_parameters p;
    u1 result[8];
    uint pc;
} simple_dalvik_vm;

/* convert to int ok */
void load_reg_to(simple_dalvik_vm *vm, int id, unsigned char *ptr);
void load_reg_to_double(simple_dalvik_vm *vm, int id, unsigned char *ptr);
void load_result_to_double(simple_dalvik_vm *vm, unsigned char *ptr);

void store_to_reg(simple_dalvik_vm *vm, int id, unsigned char *ptr);
void store_double_to_reg(simple_dalvik_vm *vm, int id, unsigned char *ptr);
void store_double_to_result(simple_dalvik_vm *vm, unsigned char *ptr);

void load_reg_to_long(simple_dalvik_vm *vm, int id, unsigned char *ptr);
void store_long_to_reg(simple_dalvik_vm *vm, int id, unsigned char *ptr);

void store_long_to_result(simple_dalvik_vm *vm, unsigned char *ptr);
void store_to_bottom_half_result(simple_dalvik_vm *vm, unsigned char *ptr);

void move_top_half_result_to_reg(simple_dalvik_vm *vm, int id);
void move_bottom_half_result_to_reg(simple_dalvik_vm *vm, int id);

void simple_dvm_startup(DexFileFormat *dex, simple_dalvik_vm *vm, char *entry);
void runMethod(DexFileFormat *dex, simple_dalvik_vm *vm, encoded_method *m);

void push(simple_dalvik_vm *vm, const u4 data);
u4 pop(simple_dalvik_vm *vm);

void invoke_clazz_method(DexFileFormat *dex, simple_dalvik_vm *vm,
                         class_data_item *clazz, invoke_parameters *p);
uint get_field_size(DexFileFormat *dex, const uint field_id);
int get_field_type(DexFileFormat *dex, const uint field_id);

sdvm_obj *create_sdvm_obj(void);
void printRegs(simple_dalvik_vm *vm);

typedef int (*opCodeFunc)(DexFileFormat *dex, simple_dalvik_vm *vm, u1 *ptr, int *pc);

typedef struct _byteCode {
    char *name;
    unsigned char opCode;
    int offset;
    opCodeFunc func;
} byteCode;

int is_verbose();
int enable_verbose();
int disable_verbose();
int set_verbose(int l);

/* sdvm internal used class for array object */
typedef enum _internal_class_type {
    ICT_NEW_FILLED_ARRAY = 1,
    ICT_NEW_ARRAY_OBJ,
    ICT_MULTI_DIM_ARRAY_OBJ,
    ICT_UNDEF_STATIC_OBJ
} internal_class_type;

typedef struct _new_filled_array {
    sdvm_obj obj;
    uint count;
    int array_elem[5];
} new_filled_array;

typedef struct _new_array_object {
    sdvm_obj obj;
    uint count;
    uint elem_size;
    u4 array[1];
} new_array_object;

typedef struct _multi_dim_array_object {
    sdvm_obj obj;
    uint count;
    uint elem_size;
    new_array_object *array[1];
} multi_dim_array_object;

typedef struct _undef_static_obj {
    sdvm_obj obj;
    uint field_id;
} undef_static_obj;

#endif
