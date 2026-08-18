# 蓝图功能提取（压缩版） / Blueprint Functional Extract

- 源文件: `Docs/tmp.txt`
- 子图(Composite): `TraceChannelAutoFind`
- 节点总数: 34  （类型 9 种）
- 原始大小: 87.6 KB / 358 行

## 设计注释（Comment 节点，原样保留）
- **[EdGraphNode_Comment_0]** IF input string is null, SET \

## 派生速查表（用于 C++ 迁移）

### 写入的变量 (VariableSet) — 5
- `"TraceChannel" (self)`  (guid 83402499)
- `"CollisionChannel" (self)`  (guid E2D85846)
- `"PreferredTraceChannelName" (self)`  (guid 1F570553)
- `"TraceChannelsSet" (self)`  (guid 37B0872A)
- `"TraceChannelsSet" (self)`  (guid 1B3BE529)

### 读取的变量 (VariableGet) — 4
- `"PreferredTraceChannelName" (self)`
- `"PreferredTraceChannelName" (self)`
- `"PreferredTraceChannelName" (self)`
- `"TraceChannel" (self)`

### 调用的函数 (Call/ArrayCall) — 4
- `Class."EqualEqual_StrStr"`
- `Class."EqualEqual_StrStr"`
- `Class."EqualEqual_StrStr"`
- `Class."EqualEqual_StrStr"`

### 类型转换 (DynamicCast) — 0

### 逻辑/数学运算 — 0

## 节点图（保留全部连接/常量/条件）

> 标记说明: `→` 表示 Pin 连线目标(节点.引脚)；`=` 表示字面常量；`exec`=执行流；`obj`=对象；`bool/int/string/byte/name`=数据类型

### K2Node_Tunnel_0  [Tunnel]  pos=-44272,-7280
  - `"execute"` (exec) → K2Node_VariableSet_0."execute"

### K2Node_Tunnel_1  [Tunnel]  pos=-41303,-6653
  - `"then"` (exec) → K2Node_VariableSet_4."then"

### K2Node_ForEachElementInEnum_0  [K2Node_ForEachElementInEnum]  pos=-42923,-7280
  - `"execute"` (exec) → K2Node_VariableSet_3."then", K2Node_Knot_4."OutputPin"
  - `"LoopBody"` (exec) → K2Node_IfThenElse_4."execute"
  - `"EnumValue"` (byte) → K2Node_GetEnumeratorNameAsString_0."Enumerator", K2Node_Knot_8."InputPin"
  - `"then"` (exec) → K2Node_Knot_6."InputPin"

### K2Node_ForEachElementInEnum_2  [K2Node_ForEachElementInEnum]  pos=-42522,-6986
  - `"execute"` (exec) → K2Node_Knot_6."OutputPin"
  - `"LoopBody"` (exec) → K2Node_IfThenElse_4."execute"
  - `"EnumValue"` (byte) → K2Node_Knot_10."InputPin"
  - `"then"` (exec) → K2Node_Knot_7."InputPin"

### K2Node_IfThenElse_5  [Branch]  pos=-42039,-7280
  - `"execute"` (exec) → K2Node_ForEachElementInEnum_0."LoopBody"
  - `"Condition"` (bool) → K2Node_CallFunction_6."ReturnValue"
  - `"then"` (exec) → K2Node_VariableSet_1."execute"
  - `"else"` (exec)

### K2Node_IfThenElse_4  [Branch]  pos=-41603,-6986
  - `"execute"` (exec) → K2Node_ForEachElementInEnum_2."LoopBody"
  - `"Condition"` (bool) → K2Node_CallFunction_7."ReturnValue"
  - `"then"` (exec) → K2Node_VariableSet_2."execute"
  - `"else"` (exec)

### K2Node_VariableSet_1  [Set]  pos=-41758,-7264
- var: `"TraceChannel" (self)`
  - `"execute"` (exec) → K2Node_IfThenElse_4."then"
  - `"then"` (exec)
  - `"TraceChannel"` (byte) → K2Node_Knot_9."OutputPin"
  - `"Output_Get"` (byte) = `"TraceTypeQuery1"`

### K2Node_VariableSet_2  [Set]  pos=-41322,-6970
- var: `"CollisionChannel" (self)`
  - `"execute"` (exec) → K2Node_IfThenElse_4."then"
  - `"then"` (exec)
  - `"CollisionChannel"` (byte) → K2Node_Knot_12."OutputPin"
  - `"Output_Get"` (byte) = `"ECC_WorldStatic"`

### K2Node_GetEnumeratorNameAsString_0  [K2Node_GetEnumeratorNameAsString]  pos=-42522,-7211
  - `"Enumerator"` (byte) → K2Node_ForEachElementInEnum_0."EnumValue"
  - `"ReturnValue"` (string) → K2Node_CallFunction_6."A"

### K2Node_GetEnumeratorNameAsString_1  [K2Node_GetEnumeratorNameAsString]  pos=-42086,-6917
  - `"Enumerator"` (byte) → K2Node_Knot_11."OutputPin"
  - `"ReturnValue"` (string) → K2Node_CallFunction_7."A"

### K2Node_CallFunction_6  [Call]  pos=-42247,-7186
- fn: `Class."EqualEqual_StrStr"`
  - `"A"` (string) → K2Node_GetEnumeratorNameAsString_0."ReturnValue"
  - `"B"` (string) → K2Node_VariableGet_4."PreferredTraceChannelName"
  - `"ReturnValue"` (bool) → K2Node_IfThenElse_4."Condition"

### K2Node_CallFunction_1  [Call]  pos=-41811,-6892
- fn: `Class."EqualEqual_StrStr"`
  - `"A"` (string) → K2Node_GetEnumeratorNameAsString_1."ReturnValue"
  - `"B"` (string) → K2Node_VariableGet_4."PreferredTraceChannelName"
  - `"ReturnValue"` (bool) → K2Node_IfThenElse_4."Condition"

### K2Node_VariableGet_6  [Get]  pos=-42511,-7125
- var: `"PreferredTraceChannelName" (self)`
  - `"PreferredTraceChannelName"` (string) → K2Node_CallFunction_6."B"

### K2Node_VariableGet_5  [Get]  pos=-43619,-7065
- var: `"PreferredTraceChannelName" (self)`
  - `"PreferredTraceChannelName"` (string) → K2Node_CallFunction_8."A"

### K2Node_CallFunction_8  [Call]  pos=-43619,-7160
- fn: `Class."EqualEqual_StrStr"`
  - `"A"` (string) → K2Node_VariableGet_5."PreferredTraceChannelName"
  - `"ReturnValue"` (bool) → K2Node_IfThenElse_6."Condition"

### K2Node_VariableSet_3  [Set]  pos=-43295,-7264
- var: `"PreferredTraceChannelName" (self)`
  - `"execute"` (exec) → K2Node_IfThenElse_6."then"
  - `"then"` (exec) → K2Node_ForEachElementInEnum_0."execute"
  - `"PreferredTraceChannelName"` (string) = `"FluidTrace"`

### K2Node_VariableGet_4  [Get]  pos=-42075,-6831
- var: `"PreferredTraceChannelName" (self)`
  - `"PreferredTraceChannelName"` (string) → K2Node_CallFunction_7."B", K2Node_CallFunction_7."A"

### K2Node_IfThenElse_6  [Branch]  pos=-43619,-7280
  - `"execute"` (exec) → K2Node_VariableSet_0."then"
  - `"Condition"` (bool) → K2Node_CallFunction_8."ReturnValue"
  - `"then"` (exec) → K2Node_VariableSet_3."execute"
  - `"else"` (exec) → K2Node_Knot_5."InputPin"

### K2Node_VariableSet_0  [Set]  pos=-44067,-7264
- var: `"TraceChannelsSet" (self)`
- note: Safety flag: OWNER will access TraceChannel data IF \
  - `"execute"` (exec) → K2Node_Tunnel_0."execute"
  - `"then"` (exec) → K2Node_IfThenElse_6."execute"

### K2Node_VariableSet_4  [Set]  pos=-41751,-6637
- var: `"TraceChannelsSet" (self)`
- note: Safety flag: OWNER will access TraceChannel data IF \
  - `"execute"` (exec) → K2Node_IfThenElse_1."then"
  - `"then"` (exec) → K2Node_Tunnel_1."then"
  - `"TraceChannelsSet"` (bool) = `"true"`

### K2Node_VariableGet_0  [Get]  pos=-42086,-6361
- var: `"TraceChannel" (self)`
  - `"TraceChannel"` (byte) → K2Node_GetEnumeratorNameAsString_2."Enumerator"

### K2Node_GetEnumeratorNameAsString_2  [K2Node_GetEnumeratorNameAsString]  pos=-42086,-6447
  - `"Enumerator"` (byte) → K2Node_VariableGet_0."TraceChannel"
  - `"ReturnValue"` (string) → K2Node_CallFunction_7."B"

### K2Node_CallFunction_7  [Call]  pos=-42086,-6533
- fn: `Class."EqualEqual_StrStr"`
  - `"A"` (string) → K2Node_VariableGet_4."PreferredTraceChannelName"
  - `"B"` (string) → K2Node_GetEnumeratorNameAsString_2."ReturnValue"
  - `"ReturnValue"` (bool) → K2Node_IfThenElse_1."Condition"

### K2Node_IfThenElse_1  [Branch]  pos=-42086,-6653
  - `"execute"` (exec) → K2Node_Knot_7."OutputPin"
  - `"Condition"` (bool) → K2Node_CallFunction_7."ReturnValue"
  - `"then"` (exec) → K2Node_VariableSet_4."execute"
  - `"else"` (exec)

### K2Node_Knot_4  [Knot]  pos=-43044,-7151
  - `"InputPin"` (exec) → K2Node_Knot_5."OutputPin"
  - `"OutputPin"` (exec) → K2Node_ForEachElementInEnum_0."execute"

### K2Node_Knot_5  [Knot]  pos=-43359,-7151
  - `"InputPin"` (exec) → K2Node_IfThenElse_6."else"
  - `"OutputPin"` (exec) → K2Node_Knot_4."InputPin"

### K2Node_Knot_6  [Knot]  pos=-42643,-6953
  - `"InputPin"` (exec) → K2Node_ForEachElementInEnum_0."then"
  - `"OutputPin"` (exec) → K2Node_ForEachElementInEnum_2."execute"

### K2Node_Knot_7  [Knot]  pos=-42211,-6620
  - `"InputPin"` (exec) → K2Node_ForEachElementInEnum_2."then"
  - `"OutputPin"` (exec) → K2Node_IfThenElse_1."execute"

### K2Node_Knot_8  [Knot]  pos=-42677,-7071
  - `"InputPin"` (byte) → K2Node_ForEachElementInEnum_0."EnumValue"
  - `"OutputPin"` (byte) → K2Node_Knot_9."InputPin"

### K2Node_Knot_9  [Knot]  pos=-41819,-7071
  - `"InputPin"` (byte) → K2Node_Knot_8."OutputPin"
  - `"OutputPin"` (byte) → K2Node_VariableSet_1."TraceChannel"

### K2Node_Knot_10  [Knot]  pos=-42271,-6777
  - `"InputPin"` (byte) → K2Node_ForEachElementInEnum_2."EnumValue"
  - `"OutputPin"` (byte) → K2Node_Knot_11."InputPin"

### K2Node_Knot_11  [Knot]  pos=-42147,-6777
  - `"InputPin"` (byte) → K2Node_Knot_10."OutputPin"
  - `"OutputPin"` (byte) → K2Node_GetEnumeratorNameAsString_1."Enumerator", K2Node_Knot_12."InputPin"

### K2Node_Knot_12  [Knot]  pos=-41383,-6777
  - `"InputPin"` (byte) → K2Node_Knot_11."OutputPin"
  - `"OutputPin"` (byte) → K2Node_VariableSet_2."CollisionChannel"
