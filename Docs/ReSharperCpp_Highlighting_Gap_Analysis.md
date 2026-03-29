# ReSharper C++ 高亮种类对标分析（Rider/UECling）

## 1. 官方已确认信息（可直接引用）

- JetBrains 文档明确写到：ReSharper C++ 支持 **20 种可分别配置颜色的 identifier types**（但文档页未逐条列出完整名称清单）。
- 配色入口在 VS 的 `Tools | Options | Environment | Fonts and Colors`，并由 ReSharper 的 `Color identifiers` 开关控制。

## 2. 社区/支持页面中可验证到的类别（非完整清单）

以下类别名称可在公开页面中直接看到（说明至少这些类别存在）：

- `ReSharper C++ Local Variable Identifier`
- `ReSharper Parameter Identifier`
- `ReSharper Method Identifier`
- `ReSharper Field Identifier`
- `ReSharper C++ Module Identifier`

> 注：完整 20 类建议后续通过导出 Rider/ReSharper 的字体颜色配置（或直接读取官方配置枚举）做一次自动抓取，避免人工漏项。

## 3. 我们当前实现能力（UECling）

当前我们已经支持两层语义来源：

1. 名称->符号种类映射（SymbolKinds）
2. SourceLocation 精确语义 token（SemanticTokens，来自编译器通道）

并在高亮阶段采用“精确 token 优先、查表兜底”的策略。

## 4. 能否达到 ReSharper 那样的丰富度？

**结论：可以接近，且在接口层面已经具备可行路径。**

要达到 ReSharper 级别，需要在 LLVM/CppInterOp 侧持续补齐：

- 更细粒度 token kind（如：静态方法、成员字段、参数、模板参数、宏、枚举成员、命名空间别名等）
- 引用角色（声明/定义/读/写/调用）
- 作用域与语境信息（类内/类外、依赖模板上下文、宏展开上下文）
- 增量更新与缓存（降低每次编辑的全量重算开销）

## 5. 分阶段落地建议

### Phase A（当前可做）
- 保持现有 SemanticTokenInfo 通道，扩展 kind 枚举
- 在编辑器端建立 `kind -> style` 映射表并允许主题覆盖

### Phase B（接近 ReSharper 体验）
- 增加角色位（declaration/reference/read/write/call）
- 增加语义修饰位（static/constexpr/mutable/template-dependent）

### Phase C（高阶体验）
- 基于 TU 缓存做增量语义 token
- 对未完成代码（incomplete code）做容错 token 生成

## 6. 风险与注意事项

- 仅靠名称查表会误判（同名符号、遮蔽、模板依赖上下文）。
- 仅靠词法 token 无法得到“符号角色”，必须依赖 AST/Sema 结果。
- 大工程中语义分析成本高，需异步+缓存。
