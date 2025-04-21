# C&C++ Secure Coding Guide

This document provides some secure coding suggestions based on the C\&C++ language to guide development.

# Functions

## Check the validity of all values received from external sources

**\[Description]**

External sources are networks, user input, command lines, files (including program configuration files), environment variables, user-mode data (for kernel programs), inter-process communications (including pipes, messages, shared memory, sockets, RPCs, and communications between different boards in a device), API parameters, and global variables.

Data from outside programs is often considered untrusted and needs to be properly checked for validity before being used. If data from an external source is not checked before use, unexpected security risks may occur.

Note: Do not use assertions to check external input data. Assertions should be used to prevent incorrect program assumptions but cannot be used to check for runtime errors in a released version.

Data from outside programs must be checked before being used. Typical scenarios include:

- **Used as an array index**

  If untrusted data is used as an array index, the array upper bound may be exceeded, causing invalid memory access. 

- **Used as a memory offset address**

  Using untrusted data as the pointer offset for memory access may result in invalid memory access and cause further damages, for example, any address read/write.

- **Used as a memory allocation size parameter** 

  Zero-byte allocation may cause invalid memory access; an unrestricted memory allocation size leads to excessive resource consumption. 

- **Used a loop condition**

  If untrusted data is used as a loop condition, problems such as buffer overflow, out-of-bounds read/write, and infinite loop may occur. 

- **Used as a divisor**

  Divide-by-zero errors may occur. 

- **Used as a command line parameter** 

  Command injection vulnerabilities may occur. 

- **Used as the parameter of a database query statement**

  SQL injection vulnerabilities may occur. 

- **Used as an input/output format string**

  Format string vulnerabilities may occur. 

- **Used as a memory copy length**

  Buffer overflows may occur. 

- **Used a file path**

  Direct access to an untrusted file path may result in directory traversal attacks. As a result, the system is controlled by the attacker who can perform file operations without permissions.

Input validation includes but is not limited to:

- API parameter validity check
- Data length check
- Data range check
- Data type and format check
- Check on inputs that can only contain permitted characters (in the trustlist), especially special characters in certain cases.

**External Data Validation Principles** 

1. Trust boundary
   
   External data is untrusted. Therefore, if data is transmitted and processed across diﬀerent trust boundaries during system operation, validity check must be performed on data from modules outside the trust boundaries to prevent attacks from spreading.
   
   (a) Diﬀerent so (or dll) modules
   
   As an independent third-party module, the so or dll module is used to export common API functions for other modules to call. The so/dll module is unable to determine whether the caller passes on valid arguments. Therefore, the common function of the so/dll module needs to check the validity of the arguments provided by the caller. The so/dll module should be designed in low coupling and high reusability. Although the so/dll module is designed to be used only in this software in certain cases, different so/dll modules should still be regarded as different trust boundaries. 
   
   (b) Diﬀerent processes 
   
   To prevent privilege escalation through processes with high permissions, the IPC communications between processes (including IPC communications between boards and network communications between hosts) should be regarded as communications across different trust boundaries. 
   
   (c) Application layer processes and operating system kernel
   The operating system kernel has higher permissions than the application layer. The interface provided by the kernel for the application layer should process the data from the application layer as untrusted data.
   
   (d) Internal and external environments of TEE
   To prevent attacks from spreading to the TEE, the interfaces provided by the TEE and SGX for external systems should process external data as untrusted data.

2. External data validation

   The external data received by a module must be validated before being used. After data validation is completed, the data stored in the module does not need to be verified again by other internal subfunctions in the module.

**\[Noncompliant Code Example]**

The **Foo()** function processes external data. Because the buﬀer does not necessarily end with '\\0', the **nameLen** value returned by **strlen** may exceed **len**. As a result, out-of-bounds read occurs.

```c
void Foo(const unsigned char *buffer, size_t len)
{
    // "buffer" may be a null pointer and may not end with '\0'.
    const char *s = (const char *)(buffer);
    size_t nameLen = strlen(s);
    ...
}
```

**\[Compliant Code Example]** 

External data is checked for validity. In this example, **strnlen** is used to calculate the string length to reduce the risk of out-of-bounds read.

```c
void Foo(const unsigned char *buffer, size_t len)
{
    // Parameter validity check must be performed.
    if (buffer == NULL || len == 0 || len >= MAX_BUFFER_LEN) {
        ... // Error handling
    }

    const char *s = (const char *)(buffer);
    size_t nameLen = strnlen(s, len); // strnlen is used to mitigate the risk of out-of-bounds read.
    if (nameLen == len) {
        ... // Error handling
    }

    Bar(name);
    ...
}
```

```c
// Bar is a common function of MODULE_B.
// If name is not NULL, the string must be a valid string, which is longer than 0 bytes and null terminated.
void Bar(const char *name)
{
    // Parameter validity check must be performed.
    if (name == NULL || name[0] == '\0') {
        // Error handling
        ...
    }
    size_t nameLen = strlen(name);  // strnlen does not need to be used.
    ...
}
```

For module A, the buffer is an external untrusted input, which must be strictly verified. Validity check is performed while the name is parsed from the buffer. The name is valid in module A, and validity check is not required when the name is transferred to internal subfunctions as a parameter. (If the name content needs to be parsed, it must be verified.) If the name in module A needs to be transferred to other modules across the trusted plane (in this example, the common function of module B is directly called, or by means of file, pipe, or network transfer), the name is untrusted data for module B and therefore validity check must be performed.

# Expressions and Statements

## Ensure that objects have been initialized before being used

**\[Description]**

Initialization is the process of setting the expected value for an object by means of explicit initialization, default constructor initialization, and value assignment. Reading an uninitialized value may result in undefined behaviour. Therefore, ensure that objects have been initialized before being used.

**\[Noncompliant Code Example]**

```c
void Bar(int data);
...
void Foo()
{
    int data;
    Bar(data); // Noncompliant: Not initialized before being used  
    ...
}
```

If there are different branches, ensure that all branches are initialized before being used as values.

```c
void Bar(int data);
...
void Foo(int condition)
{
    int data;
    if (condition > 0) {
        data = CUSTOMIZED_SIZE;
    }
    Bar(data);      // Noncompliant: Values not initialized for some branches  
    ...
}
```

**\[Compliant Code Example]**

```c
void Bar(int data);
...
void Foo()
{
    int data = 0;    // Compliant: Explicit initialization
    Bar(data);
    ...
}
void InitData(int *data);
...
void Foo()
{
    int data; 
    InitData(&data); // Compliant: Initialization using functions
    ...
}
...
```

## Ensure no overﬂows in signed integer operations

**\[Description]** 

In the C++ standard, signed integer overflow is undefined behavior. Therefore, signed integer overflows are handled differently in implementations. For example, after defining a signed integer type as a modulus, the compiler may not detect integer overflows.

Using overflowed values may cause out-of-bounds read/write risks in the buffer. For security purposes, ensure that operations do not cause overﬂows when signed integers in external data are used in the following scenarios:

- Integer operand of pointer operation (pointer offset value)
- Array index
- Length of the variable-length array (and the length operation expression)
- Memory copy length
- Parameter of the memory allocation function
- Loop judgment condition

Integer promotion needs to be considered when the operation is performed for the integer types whose precision is less than **int**. Programmers also need to master integer conversion rules, including implicit conversion rules, to design secure arithmetic operations.

**\[Noncompliant Code Example]**

In the following code example, the integers involved in the subtraction operation are external data and are not validated before being used. As a result, integer overﬂow may occur, which further results in buﬀer overﬂow due to memory copy operations.

```c
unsigned char* content = ... // Pointer to the packet header
size_t contentSize = ...     // Total length of the buffer 
int totalLen = ...           // Total length of the packet 
int skipLen = ...            // Data length that needs to be ignored from the parsed message

unsigned char *dest = ...     // Pointer to the buffer;

// Using totalLen - skipLen to calculate the length of the remaining data is likely to cause integer overflows.
memcpy(&content[skipLen], dest, , totalLen - skipLen);
...
```

**\[Compliant Code Example]** 

In the following code example, code is refactored to use the variable of the `size_t` type to indicate the data length and check whether the external data length is valid.

```c
unsigned char* content = ... // Pointer to the packet header
size_t contentSize = ...     // Total length of the buffer
size_t totalLen = ...        // Total length of the packet 
size_t skipLen = ...         // Data length that needs to be ignored from the parsed message

if (skipLen >= totalLen || totalLen > contentSize) {
    ... // Error handling
}

unsigned char *dest = ...     // Pointer to the buffer;
memcpy(&content[skipLen], dest, , totalLen - skipLen);
...
```

**\[Noncompliant Code Example]** 

In the following code example, the value range of external data is validated. However, the second type is `int`, and `std::numeric_limits<unsigned long>::max()` is incorrectly used as a validation condition. As a result, integer overflow occurs.

```c
int second = ... // External data

 //The value range of unsigned long is incorrectly used for upper limit validation.
if (second < 0 || second > (LONG_MAX / 1000)) {
    return -1;
}
int millisecond = second * 1000; // Integer overflow may occur.
...
```

**\[Compliant Code Example]** 

One option is to change the second type to `unsigned long`. This solution is applicable to the scenario where the new variable type is more fit for service logic.

```c
unsigned long second = ... // Refactor the type to unsigned long.

if (second > (LONG_MAX / 1000)) {
    return -1;
}
unsigned long millisecond = second * 1000;
...
```

Another method is to change the upper limit to `std::numeric_limits<int>::max()`.

```c
int second = ... // External data

if (second < 0 || second > (INT_MAX / 1000)) {
    return -1;
}
int millisecond = second * 1000;
```

**\[Impact]** 

Integer overflows may cause buffer overflows and arbitrary code execution.

## Ensure that unsigned integer operations do not wrap

**\[Description]** 

Integer wrap may occur in the arithmetic operation results of unsigned integers, which may cause risks such as out-of-bounds read/write in the buffer. For security purposes, ensure that operations do not cause wrapping when unsigned integers in external data are used in the following scenarios:

- Pointer offset value (integer operands in pointer arithmetic operations)
- Array index value
- Memory copy length
- Parameter of the memory allocation function
- Loop judgment condition

**\[Noncompliant Code Example]**

In the following code example, the program checks whether the total length of the next sub-packet and the processed packet exceeds the maximum packet length. The addition operation in the check condition may cause integer wrapping, causing potential validation bypassing issues.

```c
size_t totalLen = ...              // Total length of the packet 
size_t readLen = 0;                // Record the length of the processed packet.
...
size_t pktLen = ParsePktLen();     //  Length of the next sub-packet parsed from the network packet
if (readLen + pktLen > totalLen) { // Integer wrapping may occur.
  ... // Error handling
}
...
readLen += pktLen;
...
```

**\[Compliant Code Example]**

The readLen variable is the length of the processed packet and is definitely less than totalLen. Therefore, the use of the subtraction operation instead of the addition operation will not bypass the condition check.

```c
size_t totalLen = ... // Total length of the packet
size_t readLen = 0;   // Record the length of the processed packet. 
...
size_t pktLen = ParsePktLen(); // From the network packet
if (pktLen > totalLen - readLen) {
  ... // Error handling
}
...
readLen += pktLen;
...
```

**\[Noncompliant Code Example]**

In the following code example, integer wrapping may occur in the operation of len validation, resulting in condition check bypassing.

```c
size_t len =... // From the user-mode input

if (SCTP_SIZE_MAX - len < sizeof(SctpAuthBytes)) { // Integer wrapping may occur in subtraction.
    ... // Error handling
}
... = kmalloc(sizeof(SctpAuthBytes) + len, gfp);   // Integer wrapping may occur.
...
```

**\[Compliant Code Example]**

In the following code example, the subtraction operation is relocated (ensure that the value of the subtraction expression is not reversed during compilation) to avoid integer wrapping.

```c
size_t len =... // From the user-mode input

if (len > SCTP_SIZE_MAX - sizeof(SctpAuthBytes)) { // Ensure no integer wrapping for the subtraction expression value during compilation.
    ... // Error handling
}
... = kmalloc(sizeof(SctpAuthBytes) + len, gfp);
...
```

**\[Exception]**

Unsigned integers can exhibit modulo behavior (wrapping) when necessary for the proper execution of the program. It is recommended that the variable declaration and each operation on that integer be clearly commented as supporting modulo behavior.

**\[Impact]**

Integer wrapping is likely to cause buﬀer overﬂows and arbitrary code execution.

## Ensure that division and remainder operations do not cause divide-by-zero errors

**\[Description]**

Division remainder operations performed on integers with the divisor of zero are undefined behavior. Ensure that the divisor is not 0 in division and remainder operations.

The ISO/IEEE 754-1985 standard for binary floating-point arithmetic specifies the behavior and results of floating-point number division by zero. However, the presence of undefined behavior depends on whether the hardware and software environments comply with this standard. Therefore, before dividing a floating point number by zero, ensure that the hardware and software environments comply with the binary floating-point arithmetic. Otherwise, undefined behavior still exists.

**\[Noncompliant Code Example]**

```c
size_t a = ReadSize();  // From external data
size_t b = 1000 / a;    // Noncompliant: a may be 0
size_t c = 1000 % a;    // Noncompliant: a may be 0
...
```

**\[Compliant Code Example]** 

In the following code example, a=0 validation is added to prevent divide-by-zero errors.

```c
size_t a = ReadSize();  // From external data
if (a == 0) {
    ... // Error handling
}
size_t b = 1000 / a;    // Compliant: Ensure that a is not 0.
size_t c = 1000 % a;    // Compliant: Ensure that a is not 0.
...
```

**\[Impact]** 

Divide-by-zero errors are likely to cause DoS.

## Bitwise operations can be performed only on unsigned integers

**\[Description]** 

Undefined behavior may occur during bitwise operations on signed integers. To avoid undefined behavior, ensure that bitwise operations are performed only on unsigned integers. In addition, the unsigned integer type with less precision than **int** is promoted when a bitwise operation is performed on the unsigned integer. Then the bitwise operation is performed on the promoted integer. Therefore, beware of the bitwise operations on such unsigned integers to avoid unexpected results. The bitwise operators are as follows:

- `~` (Complement operator)
- `&` (AND)
- `|` (OR)
- `^` (XOR)
- `>>` (Right shift operator)
- `<<` (Left shift operator)
- `&=`
- `^=`
- `|=`
- `>>=`
- `<<=`

C++20 defines bitwise shift operations on signed integers, and such operations can be performed in compliance with C++20.

**\[Noncompliant Code Example]** 

In versions earlier than C++20, the right shift operation `data >> 24` can be implemented as arithmetic (signed) shift or logic (unsigned) shift. If the value in `value << data` is a negative number or the result of the left shift operation is out of the representable range of the promoted integer type, undefined behavior occurs.

```c
int32_t data = ReadByte();
int32_t value = data >> 24;   // The result of the right shift operation on a signed integer is implementation-defined.

... // Check the valid data range.

int32_t mask = value << data; // The left shift operation on a signed integer may cause undefined behavior.
```

**\[Compliant Code Example]**

```c
uint32_t data = (uint32_t)(ReadByte());
uint32_t value = data >> 24;  // Bitwise operations are performed only on unsigned integers.

... // Check the valid data range.

uint32_t mask  = value << data;
```

If bitwise operations are performed on unsigned integers with less precision than `int`, the operation results may be unexpected due to integer promotions. In this case, you need to immediately convert the operation results to the expected types to avoid unexpected results.

**\[Noncompliant Code Example]**

```c
uint8_t mask = 1;
uint8_t value = (~mask) >> 4; // Noncompliant: The result of the ~ operation contains high-order data, which may not meet the expectation.
```

**\[Compliant Code Example]**

```c
uint8_t mask = 1;
uint8_t value = ((uint8_t)(~mask)) >> 4; // Compliant: The result is converted to the expected type immediately after the ~ operation.
```

**\[Exception]**

- A signed integer constant or enumerated value used as a bit flag can be used as an operand for the \& and \| operators.

```c
int fd = open(fileName, O_CREAT | O_EXCL, S_IRWXU | S_IRUSR);
```

- If a signed positive integer is known at compile time, it can be used as the right operand of a shift operator.

```c
constexpr int SHIFT_BITS = 3;
...
uint32_t id = ...;
uint32_t type = id >> SHIFT_BITS;
```

# Resource Management

## Ensure validation of external data that is used as an array index or memory operation length

**\[Description]** 

When external data is used as an array index for memory access, the data size must be strictly validated to ensure that the array index is within the valid scope. Otherwise, serious errors may occur. Buffer overflows will occur if data is copied to the memory space insufficient for storing the data. To prevent such errors, limit the size of data to be copied based on the target capacity or ensure that the target capacity is sufficient to store the data to be copied.

**\[Noncompliant Code Example]** 

In the following code example, the **SetDevId()** function has an oﬀ-by-one error. When index equals `DEV_NUM`, an element is written out of bounds.

```c
struct Dev {
    int id;
    char name[MAX_NAME_LEN];
};

static Dev devs[DEV_NUM];

int SetDevId(size_t index, int id)
{
    if (index > DEV_NUM) {         // Off-by-one error
        ... // Error handling  
    }

    devs[index].id = id;
    return 0;
}
```

**\[Compliant Code Example]**

In the following code example, the index validation condition is modified to avoid the off-by-one error.

```c
struct Dev {
    int id;
    char name[MAX_NAME_LEN];
};

static Dev devs[DEV_NUM];

int SetDevId(size_t index, int id)
{
    if (index >= DEV_NUM) {
        ... // Error handling  
    }
    devs[index].id = id;
    return 0;
}
```

**\[Noncompliant Code Example]** 

External input data may not be directly used as the memory copy length, but may be indirectly involved in memory copy operations. In the following code, **inputTable.count** is from external packets. It is used as the upper limit of the **for** loop body and indirectly involved in memory copy operations, instead of being directly used as the memory copy length. Buffer overflows may occur because the length is not validated.

```c
struct ValueTable {  
    size_t count;  
    int val[MAX_NUMBERS];  
};

void ValueTableDup(const ValueTable *inputTable)  
{  
    ValueTable outputTable = {0, {0}};
    ...  
    for (size_t i = 0; i < inputTable->count; i++) {  
        outputTable.val[i] = inputTable->val[i];  
    }  
    ...  
}  
```

**\[Compliant Code Example]**

In the following code example, **inputTable.count** is validated.

```c
struct ValueTable {  
    size_t count;  
    int val[MAX_NUMBERS];  
};

void ValueTableDup(const ValueTable *inputTable)  
{  
    ValueTable outputTable = {0, {0}};  
    ...  
    // Based on application scenarios, validate the cyclic length inputTable.count of external packets
    // and the array size outputTable.val to prevent buffer overflows.
    if (inputTable->count >
        sizeof(outputTable->val) / sizeof(outputTable->val[0])) {
        ... // Error handling
    }
    for (size_t i = 0; i < inputTable.count; i++) {  
        outputTable.val[i] = inputTable->val[i];  
    }  
    ...  
}  
```

**\[Impact]**

If the length of the copied data is externally controllable, buﬀer overﬂows may occur during data copy operations, which may cause arbitrary code execution vulnerabilities.

## Verify the requested memory size before requesting memory

**\[Description]**

When the requested memory size is an external input, it must be verified to prevent the request for zero-length memory or excessive and illegal memory requests. This is because memory resources are limited and can be exhausted. If the requested memory is too large (memory requested at a time is too large, or requested multiple times in a loop), resources may be used up unexpectedly. Unexpected buffer allocation may result from incorrect parameter values, improper range checks, integer overflows, or truncation. If memory requests are controlled by attackers, security issues such as buffer overflows may occur.

**\[Noncompliant Code Example]** 

In the following code example, the memory space specified by **size** is dynamically allocated. However, **size** is not validated. 

```c
// size is not validated before being passed into to the DoSomething() function.  
int DoSomething(size_t size)
{
    ...
    char *buffer = (char *)malloc(size); // size is not validated before being used in this function.  
    ...
    free(buffer);
}
```

**\[Compliant Code Example]**

In the following code example, before the memory space specified by **size** is dynamically allocated, the validity check required by the program is performed.

```c
// size is not validated before being passed into to the DoSomething() function.  
int DoSomething(size_t size)
{
    // In this function, size is validated before being used. FOO_MAX_LEN is defined as the maximum memory space expected.
    if (size == 0 || size > FOO_MAX_LEN) {
        ... // Error handling  
    }
    char *buffer = (char *)malloc(size);
    ...
    free(buffer);
}
```

**\[Impact]** 

If the size of the requested memory is externally controllable, resources may be exhausted, resulting in DoS.

## An array should not be passed as a pointer separately when it is passed into a function as a parameter

**\[Description]** 

When the function parameter type is array (not array reference) or pointer, the array that is being passed into a function is degraded to a pointer. As a result, the array length information is lost, causing potential out-of-bounds read/write issues. If a function receives only fixed-length arrays as parameters, define the parameter type as an array reference or `std::array`. If the function receives a pointer without a length, then the length should also be passed into the function as a parameter.

**\[Noncompliant Code Example]**

```c
constexpr int MAX_LEN = 1024;
constexpr int SIZE = 10;

void UseArr(int arr[])
{
    for (int i = 0; i < MAX_LEN; i++) {
        std::cout << arr[i] << std::endl;
    }
}

void Test()
{
    int arr[SIZE] = {0};
    UseArr(arr);
}
```

**\[Compliant Code Example]**

It is easier to use the combination of the pointer and length as a type. The following is a simple encapsulation example:

```c

struct Data {
    char *buffer;
    size_t len;
};

void UseArr(const struct Data *data)
{
    if (data == NULL || data->buffer == NULL || data->len > MAX_LEN) {
        // error
        return;
    }
    for (int i = 0; i < data->len; i++) {
        printf("%x ", data->buffer[i]);
    }
    printf("\n");
}

const int SIZE = 10;

void Test()
{ 
    char arr[SIZE] = {0};
    struct Data data = {arr, len}
    UseArr(data);
}
```

If project conditions permit, it is advised to use a mature library for parameter passing, such as the `std::span` type in C++20.

If these utility classes are not used, you can pass the pointer and length as two parameters.

```c
void UseArr(int arr[], size_t len)
{
    for (int i = 0; i < len; i++) {
        std::cout << arr[i] << std::endl;
    }
}

constexpr int SIZE = 10;

void Test()
{
    int arr[SIZE] = {0};
    UseArr(arr, sizeof(arr));
}
```

## Assign a new value to the variable pointing to a resource handle or descriptor immediately after the resource is freed

**\[Description]** 

Variables pointing to resource handles or descriptors include pointers, file descriptors, socket descriptors, and other variables pointing to resources. Take a pointer as an example. If a pointer that has successfully obtained a memory segment is not immediately set to **NULL** after the memory segment is freed and no new object is allocated, the pointer is a dangling pointer. Operations on a dangling pointer may lead to double-free and access-freed-memory vulnerabilities. An effective way to mitigate these vulnerabilities is to immediately set freed pointers to new values, such as **NULL**. For a global resource handle or descriptor, a new value must be set immediately after the resource is freed, so as to prevent the invalid value from being used. For a resource handle or descriptor that is used only in a single function, ensure that the invalid value is not used again after the resource is freed.

**\[Noncompliant Code Example]** 

In the following code example, the message is processed based on the message type. After the message is processed, the memory to which the **body** points is freed, but the pointer is not set to **NULL**. If other functions process the message structure again, double-free and access-freed-memory errors may occur.

```c
int Fun()
{
    SomeStruct *msg = NULL;

    ... // Use new to allocate the memory space for msg and msg->body and initialize msg.

    if (msg->type == MESSAGE_A) {
        ...
        free(msg->body);         // Noncompliant: The pointer is not set to bNULLb after memory is freed.
    }

    ...

    // msg is saved to the global queue, and the freed body member may be used.
    if (!InsertMsgToQueue(msg)) {
        free(msg->body);         // The memory to which the body points may be freed again.
        free(msg->msg); 
        return -1;
    }
    return 0;
}
```

**\[Compliant Code Example]**

In the following code example, the released pointer is immediately set to **NULL** to avoid double-free errors.

```c
int Fun()
{
    SomeStruct *msg = NULL;

    ... // Use new to allocate the memory space for msg and msg->body and initialize msg.

    if (msg->type == MESSAGE_A) {
        ...
        free(msg->body); 
        msg->body = NULL;
    }

    ... 

    // msg saved to the global queue
    if (!InsertMsgToQueue(msg)) {
        free(msg->body);          // No need to assign NULL because the pointer leaves the scope soon
        free(msg->msg);           // No need to assign NULL because the pointer leaves the scope soon
        return -1;
    }
    return 0;
}
```

The default memory freeing function does not perform any action on NULL pointers.

**\[Noncompliant Code Example]**

In the following code example, no new value is assigned to the file descriptor after it is closed.

```c
SOCKET s = INVALID_SOCKET;
int fd = -1;
...
closesocket(s);
...
close(fd);
...
```

**\[Compliant Code Example]** 

In the following code example, a new value is assigned to the corresponding variable immediately after the resource is freed.

```c
SOCKET s = INVALID_SOCKET;
int fd = -1;
...
closesocket(s);
s = INVALID_SOCKET;
...
close(fd);
fd = -1;
...
```

**\[Impact]** 

The practices of using the freed memory, freeing the freed memory again, or using the freed resources may cause DoS or arbitrary code execution.

# Standard Library

## Use valid format strings when calling formatted input/output functions

**\[Description]** 

When using C-style formatted input/output functions, ensure that the format strings are valid and strictly match the corresponding parameter types. Otherwise, unexpected behavior occurs.

For a custom C-style formatted function, you can use the attributes supported by the compiler to automatically check its correctness. For example, the GCC can automatically check custom formatted functions (such as printf, scanf, strftime, and strfmon). For details, see Common Function Attributes in the GCC manual:

```c
extern int CustomPrintf(void* obj, const char* format, ...)
    __attribute__ ((format (printf, 2, 3)));
```

**\[Noncompliant Code Example]** 

In the following code example, an integer is formatted into the macAddr variable, but macAddr is of the unsigned char type, and %x indicates a parameter of the int type. After the function is executed, out-of-bounds write occurs.

```c
unsigned char macAddr[6];
...
// The data format in macStr is e2:42:a4:52:1e:33.
int ret = sscanf(macStr, "%x:%x:%x:%x:%x:%x\n",
                  &macAddr[0], &macAddr[1],
                  &macAddr[2], &macAddr[3],
                  &macAddr[4], &macAddr[5]);
...
```

**\[Compliant Code Example]**

In the following code example, %hhx is used to ensure that the format string strictly matches the parameter type.

```c
unsigned char macAddr[6];
...
// The data format in macStr is e2:42:a4:52:1e:33.
int ret = sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n",
                  &macAddr[0], &macAddr[1],
                  &macAddr[2], &macAddr[3],
                  &macAddr[4], &macAddr[5]);
...
```

Note: It is not advised to use C library functions, such as **sscanf()** and **sprintf()**

**\[Impact]**

An incorrect format string may cause memory damage or abnormal program termination.

## Ensure that the format parameter is not controlled by external data when a formatted input/output function is called

**\[Description]** 

When a formatted function is called, the **format** parameter provided or concatenated by external data will cause a string formatting vulnerability. Take the formatted output function of the C standard library as an example. When the **format** parameter is externally controllable, an attacker can use the %n converter to write an integer to a specified address, use the %x or %d converter to view the stack or register content, or use the %s converter to cause process crashes or other issues.

Common formatted functions are as follows:

- Formatted output functions: **sprintf()**, **vsprintf()**, **snprintf()**, **vsnprintf()**, etc.
- Formatted input functions: **sscanf()**, **vsscanf()**, **fscanf()**, **vscanf()**, etc.
- Formatted error message functions: **err()**, **verr()**, **errx()**, **verrx()**, **warn()**, **vwarn()**, **warnx()**, **vwarnx()**, **error()**, and **error\_at\_line()**
- Formatted log functions: **syslog()** and **vsyslog()**

When a formatted function is called, the constant string should be used as the format string. The format string must not be externally controllable:

```c
Box<int> v{MAX_COUNT};
std::cout << std::format("{:#x}", v);
```

**\[Noncompliant Code Example]** 

In the following code example, the **Log()** function is used to directly log external data, which may cause a format string vulnerability.

```c
void Foo()
{
    char *msg = GetMsg();
    ...
    syslog(priority, msg);       // Noncompliant: A format string vulnerability exists.  
}
```

**\[Compliant Code Example]**  
The following practice is recommended: Use the %s converter to log external data to avoid the format string vulnerability.

```c
void Foo()
{
    char *msg = GetMsg();
    ...
    syslog(priority, "%s", msg); // Compliant: No format string vulnerability exists.  
}
```

**\[Impact]**

If the format string is externally controllable, attackers can crash the process, view the stack content, view the memory content, or write data to any memory location, and then execute any code with the permission of the compromised process.

## Do not use external controllable data as parameters for process startup functions or for the loading functions of dlopen/LoadLibrary and other modules

**\[Description]** 

Process startup functions in this requirement include **system()**, **popen()**, **execl()**, **execlp()**, **execle()**, **execv()**, and **execvp()**. Each of these functions such as **system()** and **popen()** will create a process. If external controllable data is used as the parameters of these functions, injection vulnerabilities may occur. When functions such as **execl()** are used to execute new processes, command injection risks also exist if shell is used to start new processes. The use of **execlp()**, **execvp()**, and **execvpe()** functions depends on the program paths searched using the system environment variable PATH. Consider the risks of external environment variables when using these functions, or avoid using these functions.

Therefore, C standard functions are always preferred to implement the required functions. If you need to use these functions, use the trustlist mechanism to ensure that the parameters of these functions are not affected by any external data.

The **dlopen()** and **LoadLibrary()** functions load external modules. If external controllable data is used as parameters of these functions, the modules prepared by attackers may be loaded. If these functions need to be used, take one of the following measures:

- Use the trustlist mechanism to ensure that the parameters of these functions are not affected by any external data.
- Use the digital signature mechanism to protect the modules to be loaded, ensuring their integrity.
- After the security of the dynamic library loaded locally is ensured by means of permission and access control, the dynamic library is automatically loaded using a specific directory.
- After the security of the local configuration file is ensured by means of permission and access control, the dynamic library specified in the configuration file is automatically loaded.

**\[Noncompliant Code Example]**

In the following code example, external controllable data is directly used as the parameter of the **LoadLibrary()** function, which may implant Trojan horses into the program.

```c
char* msg = GetMsgFromRemote();
LoadLibrary(msg);
```

In the following code example, the external **cmd** command is executed by the **system()** function. Attackers can execute any command:

```c
std::string cmd = GetCmdFromRemote();
system(cmd.c_str());
```

In the following code example, a part of the **cmd** command executed by the **system()** function is external data. An attacker may enter `some dir;reboot` to cause system reboot:

```c
std::string name = GetDirNameFromRemote();
std::string cmd{"ls " + name};
system(cmd.c_str());
```

When using **exec()** functions to prevent command injection, do not use command parsers (such as **/bin/sh**) for the **path** and **file** parameters in the functions.

```c
int execl(const char* path, const char* arg, ...);
int execlp(const char* file, const char* arg, ...);
int execle(const char* path, const char* arg, ...);
int execv(const char* path, char* const argv[]);
int execvp(const char* file, char* const argv[]);
int execvpe(const char* file, char* const argv[], char* const envp[]);
```

For example, do not use the following methods:

```c
std::string cmd = GetDirNameFromRemote();
execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
```

Avoid calling the command processor to execute external commands if functionality can be implemented by using library functions (as shown in the preceding example). If a single command needs to be called, use the **exec\*** function for parameterized calling and implement trustlist management on the called command. In addition, avoid using the **execlp()**, **execvp()**, and **execvpe()** functions because these functions depend on the external PATH environment variable. In this case, the externally input **fileName** is only used as a parameter of the **some\_tool** command, posing no command injection risks.

```c
pid_t pid;
char *const envp[] = {NULL};
...
char *fileName = GetDirNameFromRemote();
...
pid = fork();
if (pid < 0) {
    ...
} else if (pid == 0) {
    // Use some_tool to process the specified file.
    execle("/bin/some_tool", "some_tool", fileName, NULL, envp);
    _Exit(-1);
}
...
int status;
waitpid(pid, &status, 0);
...
```

When the system command parsers (such as system) must be used to execute commands, the entered command strings must be checked based on an appropriate trustlist to prevent command injection.

```c
char *cmd = GetCmdFromRemote();

// Use the trustlist to check whether the command is valid. Only the "some_tool_a" and "some_tool_b" commands are allowed, and external control is not allowed.
if (!IsValidCmd(cmd)) {
    ... // Error handling
}
system(cmd);
...
```

**\[Impact]**

- If the command string passed to **system()**, **popen()**, or other command handler functions is externally controllable, an attacker may execute any command that exists in the system using the permission of the compromised process.
- If a dynamic library ﬁle is externally controllable, attackers can replace the dynamic library ﬁle, which may cause arbitrary code execution vulnerabilities in some cases.

## Do not apply the sizeof operator to function parameters of array type when taking the size of an array

**\[Description]**

The **sizeof** operator yields the size (in bytes) of its operand, which can be an expression or the parenthesized name of a type, for example, `sizeof(int)` or `sizeof(int *)`. Footnote 103 in section 6.5.3.4 of the C11 standard states that:

> When applied to a parameter declared to have array or function type, the **sizeof** operator yields the size of the adjusted (pointer) type.

Arguments declared as arrays in the argument list will be adjusted to pointers of corresponding types. For example, although the inArray argument in the `void Func(int inArray[LEN])` function is declared as an array, it is actually adjusted to an int pointer, that is, `void Func(int *inArray)`. As a result, `sizeof(inArray)` is equal to `sizeof(int *)` in this function, leading to unexpected result. For example, in the IA-32 architecture, `sizeof(inArray)` is 4, not the inArray size.

**\[Noncompliant Code Example]**

In the following code example, the **ArrayInit()** function is used to initialize array elements. This function has a parameter declared as `int inArray[]`. When this function is called, a 256-element integer array **data** is passed to it. The **ArrayInit()** function uses `sizeof(inArray) / sizeof(inArray[0])` to determine the number of elements in the input array. However, **inArray** is a function parameter and therefore has a pointer type. As a result, `sizeof(inArray)` is equal to `sizeof(int *)`. The expression `sizeof(inArray) / sizeof(inArray[0])` evaluates to 1, regardless of the length of the array passed to the **ArrayInit()** function, leading to unexpected behavior.

```c
#define DATA_LEN 256
void ArrayInit(int inArray[])
{
    // Noncompliant: sizeof(inArray) is used to calculate the array size.
    for (size_t i = 0; i < sizeof(inArray) / sizeof(inArray[0]); i++) {
        ...
    }
}

void FunctionData(void)
{
    int data[DATA_LEN];

    ...
    ArrayInit(data); // Call ArrayInit() to initialize array data.
    ...
}
```

**\[Compliant Code Example]**

In the following code example, the function definition is modified, an array size parameter is added, and the array size is correctly passed to the function.

```c
#define DATA_LEN 256
// Function description: Argument len is the length of inArray.
void ArrayInit(int inArray[], size_t len)
{
    for (size_t i = 0; i < len; i++) {
        ...
    }
}

void FunctionData(void)
{
    int data[DATA_LEN];

    ArrayInit(data, sizeof(data) / sizeof(data[0]));
    ...
}
```

**\[Noncompliant Code Example]**

In the following code example, `sizeof(inArray)` does not equal `ARRAY_MAX_LEN * sizeof(int)`, because the **sizeof** operator, when applied to a parameter declared to have array type, yields the size of the adjusted (pointer) type even if the parameter declaration specifies a length. In this case, `sizeof(inArray)` is equal to `sizeof(int *)`.

```c
#define ARRAY_MAX_LEN 256

void ArrayInit(int inArray[ARRAY_MAX_LEN])
{
    // Noncompliant: sizeof(inArray), pointer size rather than array size, which is not as expected
    for (size_t i = 0; i < sizeof(inArray) / sizeof(inArray[0]); i++) {
        ...
    }
}

int main(void)
{
    int masterArray[ARRAY_MAX_LEN];

    ...
    ArrayInit(masterArray);

    return 0;
}
```

**\[Compliant Code Example]**

In the following code example, the specified array length is indicated by the **len** argument.

```c
#define ARRAY_MAX_LEN 256

// Function description: Argument len is the length of the argument array.
void ArrayInit(int inArray[], size_t len)
{
    for (size_t i = 0; i < len; i++) {
        ...
    }
}

int main(void)
{
    int masterArray[ARRAY_MAX_LEN];

    ArrayInit(masterArray, ARRAY_MAX_LEN);
    ...

    return 0;
}
```

## Do not perform the **sizeof** operation on pointer variables to obtain the array size

**\[Description]** 

Performing the **sizeof** operation on a pointer that is used as an array leads to an unexpected result. For example, in the variable definition `char *p = array` where array is deﬁned as `char array[LEN]`, the result of the expression `sizeof(p)` is the same as that of `sizeof(char *)`, but not the size of the array.

**\[Noncompliant Code Example]**

In the following code example, **buﬀer** is a pointer while **path** is an array. The programmer wants to clear the two memory segments. However, the programmer mistakenly writes the memory size as `sizeof(buffer)`, leading to an unexpected result.

```c
char path[MAX_PATH];
char *buffer = (char *)malloc(SIZE);
...

...
memset(path, 0, sizeof(path));

// sizeof causes an unexpected result because its result will be the pointer size but not the buffer size.
memset(buffer, 0, sizeof(buffer));
```

**\[Compliant Code Example]** 

In the following code example, `sizeof(buffer)` is changed to the size of the requested buﬀer:

```c
char path[MAX_PATH];
char *buffer = (char *)malloc(SIZE);
...

...
memset(path, 0, sizeof(path));
memset(buffer, 0, SIZE); // Use the requested buffer size.
```

## Do not directly use external data to concatenate SQL statements

**\[Description]** 

An SQL injection vulnerability arises when an SQL query is dynamically altered to form an altogether diﬀerent query. Execution of this altered query may result in information leaks or data tampering. The root cause of SQL injection is the use of external data to concatenate SQL statements. In C/C++, external data is used to concatenate SQL statements in the following scenarios (but not limited to):

- Argument for calling **mysql\_query()** and **Execute()** when connecting to MySQL
- Argument for calling **dbsqlexec()** of the db-library driver when connecting to the SQL server
- SQL statement parameter for calling **SQLprepare()** of the ODBC driver when connecting to the database
- Argument for calling **otl\_stream()** and **otl\_column\_desc()** in OTL class library in C++ language
- Input argument for calling **ExecuteWithResSQL()** when connecting to the Oracle database in C++ language

The following methods can be used to prevent SQL injection:

- Use parameterized query (also known as a prepared statement): Parameterized query is a simple and eﬀective way to prevent SQL injection and must be used preferentially. The databases MySQL and Oracle (OCI) support parameterized query.
- Use parameterized query (driven by ODBC): supported by Oracle, SQL server, PostgreSQL, and GaussDB databases.
- Verify external data (trustlist verification is recommended).
- Escape external SQL special characters.

**\[Noncompliant Code Example]** 

The following code concatenates user input without checking the input, causing SQL injection risks:

```c
char name[NAME_MAX];
char sqlStatements[SQL_CMD_MAX];
int ret = GetUserInput(name, NAME_MAX);
...
ret = sprintf(sqlStatements,
                "SELECT childinfo FROM children WHERE name= ‘%s’",
                name);
...
ret = mysql_query(&myConnection, sqlStatements);
...
```

**\[Compliant Code Example]** 

Use prepared statements for parameterized query to defend against SQL injection attacks:

```c
char name[NAME_MAX];
...
MYSQL_STMT *stmt = mysql_stmt_init(myConnection);
char *query = "SELECT childinfo FROM children WHERE name= ?";
if (mysql_stmt_prepare(stmt, query, strlen(query))) {
    ...
}
int ret = GetUserInput(name, NAME_MAX);
...
MYSQL_BIND params[1];
(void)memset(params, 0, sizeof(params));
...
params[0].bufferType = MYSQL_TYPE_STRING;
params[0].buffer = (char *)name;
params[0].bufferLength = strlen(name);
params[0].isNull = 0;

bool isCompleted = mysql_stmt_bind_param(stmt, params);
...
ret = mysql_stmt_execute(stmt);
...
```

**\[Impact]**

If the string of a concatenated SQL statement is externally controllable, attackers can inject specific strings to deceive programs into executing malicious SQL commands, causing information leakage, permission bypass, and data tampering.

## Clear sensitive information from memory immediately after using it

**\[Description]** 

Sensitive information (such as passwords and keys) in memory must be cleared immediately after being used to prevent it from being obtained by attackers or accidentally disclosed to low-privilege users. Memory includes but is not limited to:

- Dynamically allocated memory
- Statically allocated memory
- Automatically allocated (stack) memory
- Memory cache
- Disk cache

**\[Noncompliant Code Example]** 

Generally, memory data does not need to be cleared before memory resources are released to prevent extra overheads during running. Therefore, after memory resources are released, the data remains in memory. In this case, sensitive information in the data may be leaked accidentally. To prevent sensitive information leakage, you must clear sensitive information from memory before releasing memory resources. In the following code example, the sensitive information **secret** stored in the referenced dynamic memory is copied to the newly dynamically allocated buffer **newSecret**, and is finally released through **free()**. As data is not cleared before the memory block is released, the memory block may be reallocated to another part of the program, and sensitive information stored in **newSecret** may be accidentally disclosed.

```c
char *secret = NULL;
/*
 * Assume that secret points to sensitive information whose content is less than SIZE_MAX
 * and ends with null.
 */

size_t size = strlen(secret);
char *newSecret = NULL;
newSecret = (char *)malloc(size + 1);
if (newSecret == NULL) {
    ... // Error handling
} else {
    errno_t ret = strcpy(newSecret, secret);
    ... // Process ret

    ... // Process newSecret...

    free(newSecret);
    newSecret = NULL;
}
...
```

**\[Compliant Code Example]** 

In the following code example, to prevent information leakage, clear the dynamic memory that contains sensitive information (by filling the memory space with '\\0') and then release it.

```c
char *secret = NULL;
/*
 * Assume that secret points to sensitive information whose content is less than SIZE_MAX
 * and ends with null.
 */
size_t size = strlen(secret);
char *newSecret = NULL;
newSecret = (char *)malloc(size + 1);
if (newSecret == NULL) {
    ... // Error handling
} else {
    errno_t ret = strcpy(newSecret,  secret);
    ... // Process ret

    ... // Process newSecret...

    (void)memset(newSecret,  0, size + 1);
    free(newSecret);
    newSecret = NULL;
}
...
```

**\[Compliant Code Example]** 

The following code is another scenario involving sensitive information clearance. After obtaining the password, the code saves the password to **password** for verification. After the password is used, `memset()` is used to clear the password.

```c
int Foo(void)
{
    char password[MAX_PWD_LEN];
    if (!GetPassword(password, sizeof(password))) {
        ...
    }
    if (!VerifyPassword(password)) {
        ...
    }
    ...
    (void)memset(password,  0, sizeof(password));
    ...
}
```

**NOTE**: Ensure that the code for clearing sensitive information is always valid even if the compiler has been optimized.

For example, the following code uses an invalid statement in the optimized compiler.

```c
int SecureLogin(void)
{
    char pwd[PWD_SIZE];
    if (RetrievePassword(pwd, sizeof(pwd))) {
        ... // Password check and other processing
    }
    memset(pwd, 0, sizeof(pwd)); // Compiler optimizations may invalidate this statement.
    ...
}
```

Some compilers do not execute the code during optimization if they consider the code do not change program execution results. Therefore, the **memset()** function may become invalid after optimization.

If the compiler supports the **#pragma** instruction, the instruction can be used to instruct the compiler not to optimize.

```c
void SecureLogin(void)
{
    char pwd[PWD_SIZE];
    if (RetrievePassword(pwd, sizeof(pwd))) {
        ... // Password check and other processing
    }
    #pragma optimize("", off)
    // Clear memory.
    ...
    #pragma optimize("", on)
    ...
}
```

**\[Impact]**

Failure to rapidly clear sensitive information may cause information leakage.

## Create ﬁles with appropriate access permissions explicitly specified

**\[Description]**

If no appropriate access permissions are explicitly specified when a file is created, unauthorized users may access the file, causing information leakage, file data tampering, and malicious code injection into the file.

Although ﬁle access permissions depend on the ﬁle system, many ﬁle creation functions (POSIX **open()** functions, etc.) provide mechanisms to set (or influence) ﬁle access permissions. Therefore, when these functions are used to create ﬁles, appropriate ﬁle access permissions must be explicitly specified to prevent unintended access.

**\[Noncompliant Code Example]**

The POSIX **open()** function is used to create a file but the access permission for the file is not specified, which may cause the file to be created with excessive access permissions. This may lead to vulnerabilities (e.g. CVE-2006-1174).

```c
void Foo(void)
{
    int fd = -1;
    char *filename = NULL;

    ... // Initialize filename.

    fd = open(filename, O_CREAT | O_WRONLY); // Access permission not explicitly specified 
    if (fd == -1) {
        ... // Error handling
    }
    ...
}
```

**\[Compliant Code Example]** 

Access permissions for the newly created file should be explicitly specified in the third argument to **open()**. Access permissions for a file can be set based on actual applications of the file.

```c
void Foo(void)
{
    int fd = -1;
    char *filename = NULL;

    ... // Initialize filename and specify its access permissions.

    // Explicitly specify necessary access permissions for a file.
    int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        ... // Error handling
    }
    ...
}
```

**\[Impact]**

Creating ﬁles with weak access permissions may cause unauthorized access to these ﬁles.

## Canonicalize and validate ﬁle paths before using them

**\[Description]** 

File paths from external data must be validated. Otherwise, system files may be accessed randomly. However, direct validation is not allowed. The file paths must be canonicalized before validation because a file can be described and referenced by paths in various forms. For example, a file path can be an absolute path or a relative path, and the path name, directory name, or file name may contain characters (for example, "." or "..") that make validation difficult and inaccurate. In addition, the file may also be a symbolic link, which further obscures the actual location or identity of the file, making validation difficult and inaccurate. Therefore, file paths must be canonicalized to make it much easier to validate a path, directory, or file name, thereby improving validation accuracy.

Because the canonical form may vary with operating systems and file systems, it is best to use a canonical form consistent with the current system features.

Take an example as follows:

```c
Canonicalize file paths coming from external data. Without canonicalization, attackers have chances to construct file paths for unauthorized access to files.
For example, an attacker can construct "../../../etc/passwd" to access any file.
```

**\[Noncompliant Code Example]** 

In this noncompliant code example, **inputFilename** contains a file name that originates from a tainted source, and the file is opened for writing. Before this ﬁle name is used for ﬁle operations, it should be validated to ensure that it references an expected and valid ﬁle. Unfortunately, the file name referenced by **inputFilename** may contain special characters, such as directory characters, which make validation difficult or even impossible. In addition, **inputFilename** may contain a symbolic link to any file path. Even if the file name passes validation, it is invalid. In this scenario, even if the file name is directly validated, the expected result cannot be obtained. The call to **fopen()** may result in unintended access to a file.

```c
...

if (!verify_file(inputFilename) {    // inputFilename is validated without being canonicalized.
    ... // Error handling
}

if (fopen(inputFilename, "w") == NULL) {
    ... // Error handling
}

...
```

**\[Compliant Code Example]** 

Canonicalizing file names is difficult because it requires an understanding of the underlying file system. The POSIX **realpath()** function can help convert path names to a canonical form. According to Standard for Information Technology—Portable Operating System Interface (POSIX®), Base Speciﬁcations, Issue 7 \[IEEE Std 1003.1:2013]:

- The **realpath()** function shall derive, from the pathname pointed to by **filename**, an absolute pathname that names the same file, whose resolution does not involve a dot (.), double dots (..), or symbolic links. Further verification, such as ensuring that two consecutive slashes or special files do not appear in the file name, must be performed after canonicalization. For more information about how to perform path name resolution, see section 4.12 "Pathname Resolution" of IEEE Std 1003.1:2013. There are many precautions for using the **realpath()** function.  With an understanding of the preceding principles, the following solution can be taken to address the noncompliant code example.

```c
char *realpathRes = NULL;

...

// Canonicalize inputFilename before validation.
realpathRes = realpath(inputFilename, NULL);
if (realpathRes == NULL) {
    ... // Canonicalization error handling
}

// Validate the file path after canonicalizing it
if (!verify_file(realpathRes) {
    ... // Validation error handling 
}

// Use
if (fopen(realpathRes, "w") == NULL) {
    ... // Operation error handling
}

...

free(realpathRes);
realpathRes = NULL;
...
```

**\[Compliant Code Example]**

Based on the actual scenario, a second solution can also be used. The description is as follows: If `PATH_MAX` is deﬁned as a constant in **limits.h**, it is also safe to call **realpath()** with a non-null `resolved_path` value. In this example, the **realpath()** function expects `resolved_path` to refer to a character array that is large enough to hold the canonicalized path. If **PATH\_MAX** is deﬁned, allocate a buﬀer of size `PATH_MAX` to hold the result of **realpath()**. The compliant code example is as follows:

```c
char *realpathRes = NULL;
char *canonicalFilename = NULL;
size_t pathSize = 0;

...

pathSize = (size_t)PATH_MAX;

if (VerifyPathSize(pathSize)) {
    canonicalFilename = (char *)malloc(pathSize);

    if (canonicalFilename == NULL) {
        ... // Error handling
    }

    realpathRes = realpath(inputFilename, canonicalFilename);
}

if (realpathRes == NULL) {
    ... // Error handling
}

if (VerifyFile(realpathRes)) {
    ... // Error handling
}

if (fopen(realpathRes, "w") == NULL ) {
    ... // Error handling
}

...

free(canonicalFilename);
canonicalFilename = NULL;
...
```

**\[Noncompliant Code Example]** 

The following code obtains file names from external data, concatenates them into a file path, and directly reads the file content. As a result, attackers can read the content of any file.

```c
char *filename = GetMsgFromRemote();
...
int ret = sprintf(untrustPath,  "/tmp/%s", filename);
...
char *text = ReadFileContent(untrustPath);
```

**\[Compliant Code Example]**

Canonicalize the file path and then check whether the path is valid in the program.

```c
char *filename = GetMsgFromRemote();
...
sprintf(untrustPath,  "/tmp/%s", filename);
char path[PATH_MAX];
if (realpath(untrustPath, path) == NULL) {
    ... // Error handling
}
if (!IsValidPath(path)) {    // Check whether the file path is valid.
    ... // Error handling
}
char *text = ReadFileContent(path);
```

**\[Exception]**

File paths can be manually entered on the console where the command line program runs.

```c
int main(int argc, char **argv)
{
    int fd = -1;

    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        ...
    }

    ...
    return 0;
}
```

**\[Impact]**

Failure to canonicalize and validate untrusted file paths may cause access to any file.

## Do not create temporary ﬁles in shared directories

**\[Description]** 

A shared directory refers to a directory that can be accessed by non-privileged users. The temporary ﬁles of a program must be exclusively used by the program. If you place the temporary ﬁles of the program in the shared directory, other sharing users may obtain additional information about the program, resulting in information leakage. Therefore, do not create temporary files that are used only by a program itself in any shared directory.

Temporary ﬁles are commonly used for auxiliary storage of data that cannot reside in memory or temporary data and also as a means of inter-process communication (by transmitting data through the ﬁle system). For example, one process creates a temporary ﬁle with a well-known name or a temporary name in a shared directory. The ﬁle can then be used to share information among processes. This practice is dangerous because files in a shared directory can be easily hijacked or manipulated by an attacker. Mitigation strategies include the following:

1. Use other low-level inter-process communication (IPC) mechanisms, such as sockets or shared memory.
2. Use higher-level IPC mechanisms, such as remote procedure call (RPC).
3. Use secure directories that can be accessed only by a program itself (Avoid race conditions in the case of multiple threads or processes.)

The following lists several methods for creating temporary files. Product teams can use one or more of these methods as required or customize their own methods.

1. Files must have appropriate permissions so that they can be accessed only by authorized users.
2. The name of a created file is unique or unpredictable.
3. Files can be created and opened only if the files do not exist (atomic create and open).
4. Open the files with exclusive access to avoid race conditions.
5. Remove files before the program exits.

In addition, when two or more users or a group of users have read/write permission to a directory, the potential security risk of the shared directory is far greater than that of the access to temporary ﬁles in the directory.

Creating temporary files in a shared directory is vulnerable. For example, code that works for a locally mounted file system may be vulnerable when shared with a remotely mounted ﬁle system. The secure solution is not to create temporary ﬁles in shared directories.

**\[Noncompliant Code Example]**

The program creates a temporary file with a hard-coded file name in the shared directory **/tmp**  to store temporary data. Because the ﬁle name is hard-coded and consequently predictable, an attacker only needs to replace the ﬁle with a symbolic link. The target ﬁle referenced by the link is then opened and new content can be written.

```c
void ProcData(const char *filename)
{
    FILE *fp = fopen(filename, "wb+");
    if (fp == NULL) {
        ... // Error handling
    }

    ... // Write a file.

    fclose(fp);
}

int main(void)
{
    // Noncompliant: 1. A temporary file is created in shared directories. 2. The temporary file name is hard-coded.
    char *pFile = "/tmp/data";
    ...

    ProcData(pFile);

    ...
    return 0;
}
```

**\[Compliant Code Example]**

```c
Do not create temporary files that are used only by a program itself in this directory.
```

**\[Impact]**

Creating temporary ﬁles in an insecure manner may cause unauthorized access to the ﬁles and privilege escalation in the local system.

## Do not access shared objects in signal handlers

**\[Description]** 

Accessing or modifying shared objects in signal handlers can result in race conditions that can leave data in an uncertain state. This rule is not applicable to the following scenarios (see paragraph 5 in section 5.1.2.3 of the C11 standard):

- Read/write operations on lock-free atomic object
- Read/write operations on objects of the **volatile sig\_atomic\_t** type. An object of the **volatile sig\_atomic\_t** type can be accessed as an atomic entity even in the presence of asynchronous interrupts, and is asynchronous-safe.

**\[Noncompliant Code Example]** 

In the signal handler, the program attempts to use `g_msg` as the shared object and update the shared object when the SIGINT signal is delivered. However, `g_msg` is not a variable of type `volatile sig_atomic_t`, and is not asynchronous-safe.

```c
#define MAX_MSG_SIZE 32
static char g_msgBuf[MAX_MSG_SIZE] = {0};
static char *g_msg = g_msgBuf;

void SignalHandler(int signum)
{
    // It is noncompliant to use g_msg because it is not asynchronous-safe.
    (void)memset(g_msg,0, MAX_MSG_SIZE);
    errno_t ret = strcpy(g_msg,  "signal SIGINT received.");
    ... // Process ret
}

int main(void)
{
    errno_t ret = strcpy(g_msg,  "No msg yet."); // Initialize message content.
    ... // Process ret

    signal(SIGINT, SignalHandler); // Set the SIGINT signal handler.

    ... // Main code loop

    return 0;
}
```

**\[Compliant Code Example]** 

In the following code example, only the `volatile sig_atomic_t` type is used as a shared object in signal handlers.

```c
#define MAX_MSG_SIZE 32
volatile sig_atomic_t g_sigFlag = 0;

void SignalHandler(int signum)
{
    g_sigFlag = 1; // Compliant
}

int main(void)
{
    signal(SIGINT, SignalHandler);
    char msgBuf[MAX_MSG_SIZE];
    errno_t ret = strcpy(msgBuf, "No msg yet."); // Initialize message content.
    ... // Process ret

    ... // Main code loop

    if (g_sigFlag == 1) {  // Update message content based on g_sigFlag status after exiting the main loop.
        ret = strcpy(msgBuf,  "signal SIGINT received.");
        ... // Process ret
    }

    return 0;
}
```

**\[Impact]**

Accessing or modifying shared objects in signal handlers may cause inconsistent status access data.

## Do not use rand() to generate pseudorandom numbers for security purposes

**\[Description]** 

The **rand()** function in the C language standard library generates pseudorandom numbers, which does not ensure the quality of the random sequence produced. In the C11 standard, the range of random numbers generated by the **rand()** function is `[0, RAND_MAX(0x7FFF)]`, which has a relatively short cycle, and the numbers can be predictable. Therefore, do not use the random numbers generated by the **rand()** function for security purposes. Use secure random number generation methods.

Typical scenarios for security purposes include but are not limited to the following:

- Generation of session IDs;
- Generation of random numbers in the challenge algorithm;
- Generation of random numbers of verification codes;
- Generation of random numbers for cryptographic algorithm purposes (for example, generating IVs, salt values, and keys).

**\[Noncompliant Code Example]** 

The programmer wants the code to generate a unique HTTP session ID that is not predictable. However, the ID is a random number produced by calling the **rand()** function, and is predictable and has limited randomness.

**\[Impact]**

Using the **rand()** function may result in random numbers that are predictable.

## Do not output the address of an object or function in a released version

**\[Description]** 

Do not output the address of an object or function in a released version. For example, do not output the address of a variable or function to a client, log, or serial port.

Before launching an advanced attack, the attacker usually needs to obtain the memory address (such as the variable address and function address) of the target program and then modify the content of the specified memory for attacks. If the program itself outputs the addresses of objects or functions, the attacker can take this advantage and use these addresses and oﬀsets to calculate the addresses of other objects or functions and launch attacks. In addition, the protection function of address space randomization also fails due to memory address leakage.

**\[Noncompliant Code Example]** 

In the following code example, the address to which the pointer points is logged in the %p format.

```c
int Encode(unsigned char *in, size_t inSize, unsigned char *out, size_t maxSize)
{
    ...
    Log("in=%p, in size=%zu, out=%p, max size=%zu\n", in, inSize, out, maxSize);
    ...
}
```

Note: This example uses only the %p format for logging pointers. In scenarios where pointers are converted to integers and then logged, the same risk exists.

**\[Compliant Code Example]**

In the following code example, the code for logging the address is deleted.

```c
int Encode(unsigned char *in, size_t inSize, unsigned char *out, size_t maxSize)
{
    ...
    Log("in size=%zu, max size=%zu\n", inSize, maxSize);
    ...
}
```

**\[Exception]** 

When the program crashes and exits, the memory address and other information can be output in the crash exception information.

**\[Impact]**

Memory address leakage creates vulnerabilities to adversaries, probably leading to an address space randomization protection failure.

## Do not include public IP addresses in code

**\[Description]**

If the public IP addresses that are invisible and unknown to users are included in code or scripts, customers may doubt code security.

Public network addresses (including public IP addresses, public URLs/domain names, and email addresses) contained in the released software (including software packages and patch packages) must meet the following requirements: 1\. Do not contain any public network address that is invisible on UIs or not disclosed in product documentation. 2\. Do not write disclosed public IP addresses in code or scripts. They can be stored in configuration files or databases.

The public IP addresses built in open-source or third-party software must meet the first requirement at least.

**\[Exception]**

This requirement is not mandatory when public network addresses must be specified as required by standard protocols. For example, an assembled public network URL must be specified for the namespace of functions based on the SOAP protocol. W3.org addresses on HTTP pages are also exceptions.

# Secure Kernel Coding

## Ensure that the mapping start address and space size in kernel mmap are validated

**\[Description]**

In the mmap interface of the  kernel, the **remap\_pfn\_range()** function is often used to map the physical memory of a device to a user process space. If the parameters (such as the mapping start address) are controlled by the user mode and no validation is performed, the user mode can read and write any kernel address through the mapping. An attacker can even construct arguments to run arbitrary code in the kernel.

**\[Noncompliant Code Example]**

When **remap\_pfn\_range()** is used for memory mapping in the following code, the user-controllable mapping start address and space size are not validated. As a result, the kernel may crash or any code may be executed.

```c
static int incorrect_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size;
	size = vma->vm_end - vma->vm_start;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	// Error: The mapping start address and space size are not validated.
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) { 
		err_log("%s, remap_pfn_range fail", __func__);
		return EFAULT;
	} else {
		vma->vm_flags &=  ~VM_IO;
	}

	return EOK;
}
```

**\[Compliant Code Example]**

Add the validity check on parameters such as the mapping start address.

```c
static int correct_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size;
	size = vma->vm_end - vma->vm_start;
	// Modification: Add a function to check whether the mapping start address and space size are valid.
	if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size)) { 
		return EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		err_log( "%s, remap_pfn_range fail ", __func__);
		return EFAULT;
	} else {
		vma->vm_flags &=  ~VM_IO;
	}

	return EOK;
}
```

## Kernel programs must use kernel-specific functions to read and write user-mode buffers

**\[Description]**

During data exchange between the user mode and kernel mode, if no check (such as address range check and null pointer check) is performed in the kernel and the user mode input pointer is directly referenced, the kernel may crash and any address may be read or written when an invalid pointer is input in the user mode. Therefore, do not use unsafe functions such as **memcpy()** and **sprintf()**. Instead, use the dedicated functions provided by the kernel, such as **copy\_from\_user()**, **copy\_to\_user()**, **put\_user()**, and **get\_user()**, to read and write the user-mode buffer. Input validation is added to these functions.

The forbidden functions are **memcpy()**, **bcopy()**, **memmove()**, **strcpy()**, **strncpy()**, **strcat()**, **strncat()**, **sprintf()**, **vsprintf()**, **snprintf()**, **vsnprintf()**, **sscanf()** and **vsscanf()**.

**\[Noncompliant Code Example]**

The kernel mode directly uses the buf pointer input by the user mode as the argument of **snprintf()**. When **buf** is **NULL**, the kernel may crash.

```c
ssize_t incorrect_show(struct file *file, char__user *buf, size_t size, loff_t *data)
{
	// Error: The user-mode pointer is directly referenced. If the value of buf is NULL, a null pointer causes kernel crashes.
	return snprintf(buf, size, "%ld\n", debug_level); 
}
```

**\[Compliant Code Example]**

Use the **copy\_to\_user()** function instead of **snprintf()**.

```c
ssize_t correct_show(struct file *file, char __user *buf, size_t size, loff_t *data)
{
	int ret = 0;
	char level_str[MAX_STR_LEN] = {0};
	snprintf(level_str, MAX_STR_LEN, "%ld \n", debug_level);
	if(strlen(level_str) >= size) {
		return EFAULT;
	}
	
	// Modification: Use the dedicated function copy_to_user() to write data to the user-mode buf and prevent buffer overflow.
	ret = copy_to_user(buf, level_str, strlen(level_str)+1); 
	return ret;
}
```

**\[Noncompliant Code Example]**

The pointer **user\_buf** transferred in user mode is used as the data source to perform the **memcpy()** operation in kernel mode. When **user\_buf** is **NULL**, the kernel may crash.

```c
size_t incorrect_write(struct file  *file, const char __user  *user_buf, size_t count, loff_t  *ppos)
{
	...
	char buf [128] = {0};
	int buf_size = 0;
	buf_size = min(count, (sizeof(buf)-1));
	// Error: The user-mode pointer is directly referenced. If user_buf is NULL, the kernel may crash.
	(void)memcpy(buf, user_buf, buf_size); 
	...
}
```

**\[Compliant Code Example]**

Replace **memcpy()** with **copy\_from\_user()**.

```c
ssize_t correct_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	...
	char buf[128] = {0};
	int buf_size = 0;

	buf_size = min(count, (sizeof(buf)-1));
	// Modification: Use the dedicated function copy_from_user() to write data to the kernel-mode buf and prevent buffer overflows.
	if (copy_from_user(buf, user_buf, buf_size)) { 
		return EFAULT;
	}

	...
}
```

## Verify the copy length of **copy\_from\_user()** to prevent buffer overflows

**\[Description]**

The **copy\_from\_user()** function is used in kernel mode to copy data from the user mode. If the length of the copied data is not validated or is improperly validated, the kernel buffer overflows, causing kernel panic or privilege escalation.

**\[Noncompliant Code Example]**

The copy length is not validated.

```c
static long gser_ioctl(struct file  *fp, unsigned cmd, unsigned long arg)
{
	char smd_write_buf[GSERIAL_BUF_LEN];
	switch (cmd)
	{
		case GSERIAL_SMD_WRITE:
			if (copy_from_user(&smd_write_arg, argp, sizeof(smd_write_arg))) {...}
			// Error: The value of smd_write_arg.size is entered by the user and is not validated.
			copy_from_user(smd_write_buf, smd_write_arg.buf, smd_write_arg.size); 
			...
	}
}
```

**\[Compliant Code Example]**

Length validation is added.

```c
static long gser_ioctl(struct file *fp, unsigned cmd, unsigned long arg)
{
	char smd_write_buf[GSERIAL_BUF_LEN];
	switch (cmd)
	{
		case GSERIAL_SMD_WRITE:
			if (copy_from_user(&smd_write_arg, argp, sizeof(smd_write_arg))){...}
			// Modification: Add validation.
			if (smd_write_arg.size  >= GSERIAL_BUF_LEN) {......} 
			copy_from_user(smd_write_buf, smd_write_arg.buf, smd_write_arg.size);
 			...
	}
}
```

## Initialize the data copied by **copy\_to\_user()** to prevent information leakage

**\[Description]**

**Note:** When **copy\_to\_user()** is used in kernel mode to copy data to the user mode, sensitive information (such as the pointer on the stack) may be leaked if the data is not completely initialized (for example, the structure member is not assigned a value, or the memory fragmentation is caused by byte alignment). Attackers can bypass security mechanisms such as Kaslr.

**\[Noncompliant Code Example]**

The data structure members are not completely initialized.

```c
static long rmnet_ctrl_ioctl(struct file *fp, unsigned cmd, unsigned long arg)
{
	struct ep_info info;
	switch (cmd) {
		case FRMNET_CTRL_EP_LOOKUP:
			info.ph_ep_info.ep_type = DATA_EP_TYPE_HSUSB;
			info.ipa_ep_pair.cons_pipe_num = port->ipa_cons_idx;
			info.ipa_ep_pair.prod_pipe_num = port->ipa_prod_idx;
			// Error: The info structure has four members, not all of which are assigned with values.
			ret = copy_to_user((void __user *)arg, &info, sizeof(info)); 
			...
	}
}
```

**\[Compliant Code Example]**

Initialize all data.

```c
static long rmnet_ctrl_ioctl(struct file *fp, unsigned cmd, unsigned long arg)
{
	struct ep_info info;
	// Modification: Use memset to initialize the buffer to ensure that no memory fragmentation exists due to byte alignment or no value assignment.
	(void)memset(&info, '0', sizeof(ep_info)); 
	switch (cmd) {
		case FRMNET_CTRL_EP_LOOKUP:
			info.ph_ep_info.ep_type = DATA_EP_TYPE_HSUSB;
			info.ipa_ep_pair.cons_pipe_num = port->ipa_cons_idx;
			info.ipa_ep_pair.prod_pipe_num = port->ipa_prod_idx;
			ret = copy_to_user((void __user *)arg, &info, sizeof(info));
			...
	}
}
```

## Do not use the BUG\_ON macro in exception handling to avoid kernel panic

**\[Description]**

The BUG\_ON macro calls the **panic()** function of the kernel to print error information and suspend the system. In normal logic processing (for example, the **cmd** parameter of the **ioctl** interface cannot be identified), the system should not crash. Do not use the BUG\_ON macro in such exception handling scenarios. The WARN\_ON macro is recommended.

**\[Noncompliant Code Example]**

The BUG\_ON macro is used in the normal process.

```c
/ * Determine whether the timer on the Q6 side is busy. 1: busy; 0: not busy */
static unsigned int is_modem_set_timer_busy(special_timer *smem_ptr)
{
	int i = 0;
	if (smem_ptr == NULL) {
		printk(KERN_EMERG"%s:smem_ptr NULL!\n", __FUNCTION__);
		// Error: The system BUG_ON macro calls panic() after printing the call stack, which causes kernel DoS and should not be used in normal processes.
		BUG_ON(1); 
		return 1;
	}

	...
}
```

**\[Compliant Code Example]**

Remove the BUG\_ON macro.

```c
/ * Determine whether the timer on the Q6 side is busy. 1: busy; 0: not busy */
static unsigned int is_modem_set_timer_busy(special_timer *smem_ptr)
{
	int i = 0;
	if (smem_ptr == NULL) {
		printk(KERN_EMERG"%s:smem_ptr NULL!\n",  __FUNCTION__);
		// Modification: Remove the BUG_ON call or use WARN_ON.
		return 1;
	}

	...
}
```

## Do not use functions that may cause the process hibernation in the interrupt handler or in the context code of the process that holds the spin lock

**\[Description]**

Processes as the scheduling unit. In the interrupt context, only the interrupt with a higher priority can be interrupted. The system cannot schedule processes during interrupt processing. If the interrupt handler is in hibernation state, the kernel cannot be woken up, paralyzing the kernel.

Spin locks disable preemption. If the spin lock enters the hibernation state after being locked, other processes will stop running because they cannot obtain the CPU (single-core CPU). In this case, the system does not respond and is suspended.

Therefore, functions that may cause hibernation (such as **vmalloc()** and **msleep()**), block (such as **copy\_from\_user()**, **copy\_to\_user()**), or consume a large amount of time (such as **printk()**) should not be used in the interrupt processing program or the context code of the process that holds the spin lock.

## Use the kernel stack properly to prevent kernel stack overflows

**\[Description]**

The kernel stack size is fixed (8 KB for a 32-bit system and 16 KB for a 64-bit system). Improper use of the kernel stack may cause stack overflows and system suspension. Therefore, the following requirements must be met:

- The memory space requested on the stack cannot exceed the size of the kernel stack.
- Pay attention to the number of function nestings.
- Do not define excessive variables.

**\[Noncompliant Code Example]**

The variables defined in the following code are too large, causing stack overflows.

```c
...
struct result
{
	char name[4];
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;
}; // The size of the result structure is 20 bytes.

int foo()
{
	struct result temp[512];
	// Error: The temp array contains 512 elements. The total size is 10 KB, which is far greater than the kernel stack size.
	(void)memset(temp, 0, sizeof(result) * 512); 
	... // use temp do something
	return 0;
}

...
```

The **temp** array has 512 elements, and the total size is 10 KB, which is far greater than the kernel size (8 KB). The stack overflows obviously.

**\[Compliant Code Example]**

Use **kmalloc()** instead.

```c
...
struct result
{
	char name[4];
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;
}; // The size of the result structure is 20 bytes.

int foo()
{
	struct result  *temp = NULL;
	temp = (result *)kmalloc(sizeof(result) * 512, GFP_KERNEL); // Modification: Use kmalloc() to apply for memory.
	... // check temp is not NULL
	(void)memset(temp, 0, sizeof(result)  * 512);
	... // use temp do something
	... // free temp
	return 0;
}
...
```

## Restore address validation after the operation is complete

**\[Description]**

The SMEP security mechanism prevents the kernel from executing the code in the user space (PXN is the SMEP of the ARM version). System calls (such as **open()** and **write()**) are originally provided for user space programs to access. By default, these functions validate the input address. If it is not a user space address, an error is reported. Therefore, disable address validation before using these system calls in a kernel program. **set\_fs()**/**get\_fs()** is used to address this problem. For details, see the following code:

```c
...
mmegment_t old_fs;
printk("Hello, I'm the module that intends to write message to file.\n");
if (file == NULL) {
	file = filp_open(MY_FILE, O_RDWR | O_APPEND | O_CREAT, 0664);
}

if (IS_ERR(file)) {
	printk("Error occurred while opening file %s, exiting ...\n", MY_FILE);
	return 0;
}

sprintf(buf, "%s", "The Message.");
old_fs = get_fs(); // get_fs() is used to obtain the upper limit of the user space address.  
                   // #define get_fs() (current->addr_limit
set_fs(KERNEL_DS); // set_fs is used to increase the upper limit of the address space to KERNEL_DS so that the kernel code can call system functions.
file->f_op->write(file, (char *)buf, sizeof(buf), &file->f_pos); // The kernel code can call the write() function.
set_fs(old_fs); // Restore the address limit of the user space in time after use.
...
```

According to the preceding code, it is vital to restore address validation immediately after the operation is complete. Otherwise, the SMEP/PXN security mechanism will fail, making it easy to exploit many vulnerabilities.

**\[Noncompliant Code Example]**

The program error processing branch does not use **set\_fs()** to restore address validation.

```c
...
oldfs = get_fs();
set_fs(KERNEL_DS);
/* Create a done file in the timestamp directory. */
fd = sys_open(path, O_CREAT | O_WRONLY, FILE_LIMIT);
if (fd < 0) {
	BB_PRINT_ERR("sys_mkdir[%s] error, fd is[%d]\n", path, fd);
	return; // Error: Address validation is not restored in the error processing program branch.
}

sys_close(fd);
set_fs(oldfs);
...
```

**\[Compliant Code Example]**

Address validation is restored in the error processing program.

```c
...
oldfs = get_fs();
set_fs(KERNEL_DS);

/* Create a done file in the timestamp directory. */
fd = sys_open(path, O_CREAT | O_WRONLY, FILE_LIMIT);
if (fd < 0) {
	BB_PRINT_ERR("sys_mkdir[%s] error, fd is[%d] \n", path, fd);
	set_fs(oldfs); // Modification: Restore address validation in the error processing program branch.
	return;
}

sys_close(fd);
set_fs(oldfs);
...
```