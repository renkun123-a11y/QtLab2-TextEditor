# 开发与缺陷记录

按时间顺序记录这次实验里真正踩到的坑：怎么发现、为什么、怎么改、改完怎么验证。
测试输入都是实际跑出来的，不是事后编的。

## 1. Qt 6 里没有 QTextCodec，GBK 怎么办

**问题**：最早按 Qt 5 的写法用 `QTextCodec::codecForName("GBK")` 做编码识别，
编译直接失败——Qt 6 把 QTextCodec 挪到了 Qt5Compat 模块，而本机安装没有这个模块。

**原因**：`QStringConverter` 只内置 UTF-8 / UTF-16 / UTF-32 / Latin1 / System，
GBK 不在其中；不装 ICU 时 `QStringDecoder("GBK")` 也拿不到编码器。

**修改**：`fileio.cpp` 里分开处理：UTF-8 用 `QStringDecoder` 严格解码
（`hasError()` 判断是否合法），GBK 走 Windows 的代码页 936
（`MultiByteToWideChar` / `WideCharToMultiByte`），并用 `MB_ERR_INVALID_CHARS`
和 `WC_NO_BEST_FIT_CHARS` 做严格判断。

**验证**：`tst_editor` 的 `gbkDetection` / `gbkRoundTrip` 用例通过；
`samples/sample_gbk.txt` 打开后中文正常，状态栏显示 GBK，另存后仍是 GBK。

## 2. 二进制判断把 UTF-16 文件误杀了

**问题**：先查“数据里有没有 0x00”，结果 UTF-16 文件天然带 0x00，被当成二进制拒绝。

**原因**：UTF-16 每个 ASCII 字符的高字节都是 0，NUL 判断必须在 BOM 分支之后。

**修改**：`readTextFile()` 的顺序固定为：UTF-8 BOM → UTF-16 LE BOM →
UTF-16 BE BOM → 查 NUL → 严格 UTF-8 → GBK。

**验证**：构造 UTF-16 文件可以正常打开；`samples/fake_binary.dat`（512 字节、
含 NUL）提示“看起来是二进制文件”，编辑器内容不变。

## 3. 保存到只读文件时，提示是英文且语义不清

**问题**：把 `samples/readonly.txt` 设为只读后按 Ctrl+S，弹窗内容是
`Existing file ... is not writable`，用户看不出发生了什么。

**原因**：`QSaveFile::commit()` 把系统的错误串原样返回，没有做场景化处理。

**修改**：`writeFile()` 里识别 `not writable` / `Permission denied`，换成
“文件是只读的，没有写入权限。”；同时保持 `document()->setModified(true)` 不变，
标题上的 `*` 不消失，避免“假保存成功”。

**验证**：截图 25；`tst_editor::writeFailureKeepsOriginal` 断言写入失败后
文件内容仍是“原始内容”。

## 4. 替换到最后一处时会弹“未找到”

**问题**：连续点“替换”，替换掉最后一处后，`replaceOnce()` 为了选中下一个又调用
`findText()`，找不到就弹“未找到”，打断连续替换。

**原因**：`findText()` 同时承担“查找”和“提示”两个职责，替换流程复用了它。

**修改**：拆出 `findNext(text, caseSensitive, backward, notify)`，`findText()` 传
`true`，`replaceOnce()` 传 `false`；`replaceAll()` 不受影响。

**验证**：`tst_editor::replaceOnceFollowsSelection`；手动连续替换三次 hello→HI，
第三次之后不再弹窗。

## 5. 标题的 `*` 号：setWindowModified 的警告

**问题**：控制台反复打印
`QWidget::setWindowModified: The window title does not contain a '[*]' placeholder`。

**原因**：标题是手动拼的 `* 文件名 - 程序名`，没有 `[*]` 占位符，
`setWindowModified()` 无处可写。

**修改**：直接按 `isModified()` 拼前缀，去掉 `setWindowModified()` 调用。
`QTextDocument::modificationChanged` 仍然接到 `updateWindowTitle()`，保存 / 撤销后标题立即更新。

**验证**：截图 6（保存后无 `*`）、截图 7（编辑后有 `*`）、截图 18（撤销后 `*` 消失）。

## 6. 回车会触发两次查找

**问题**：早期版本给查找输入框连了 `returnPressed`，同时“查找下一个”是默认按钮，
按一次回车前进两格。

**原因**：QLineEdit 没有接受回车事件时会继续传给 QDialog，QDialog 把它解释为
点击默认按钮，于是 `returnPressed` 与按钮 `clicked` 各触发一次。

**修改**：不连 `returnPressed`，只保留默认按钮；替换对话框同样处理。

**验证**：连续按回车依次命中第 1、2、3 个 hello，第 4 次提示“已到文档末尾，从开头继续查找”。

## 7. 循环查找的边界

**问题**：从文档末尾开始查找时，第一轮 `find()` 必然失败。

**原因**：`QPlainTextEdit::find()` 从当前光标向后找，不会自动回绕。

**修改**：第一轮失败后把光标移到文档首 / 尾再找一次，成功则提示“已到文档末尾/开头”，
两次都失败才提示“未找到”。

**验证**：截图 15、16；`tst_editor::findWrapsAround` 断言第 4 次查找回到第 1 个块。

## 8. 行号宽度与 Tab 宽度

**问题**：行数从 9 行变成 10 行时，行号区宽度不变，两位数行号被截断；
换成比例字体后 Tab 缩进错位。

**原因**：行号宽度只在构造时算了一次；`setTabStopDistance` 用的是默认字体的度量。

**修改**：`lineNumberAreaWidth()` 按 `blockCount()` 的位数算；`setEditorFont()`
里重新设置 Tab 宽度（4 个空格宽）并 `updateLineNumberAreaWidth()`。

**验证**：`tst_editor::lineNumberWidthGrowsWithDigits`：1 行与 120 行的宽度不同，
隐藏行号后宽度为 0；截图 19。

## 9. 当前行高亮盖住选区

**问题**：查找选中文字后，整行高亮画在选区之上，看起来像“没有选中”。

**原因**：`ExtraSelection` 画在文本选区之后。

**修改**：有选区时不画当前行高亮；同时把 `Inactive` 状态的 `Highlight` 调色板设成
与 `Active` 一致，对话框弹出、窗口失焦时选区仍然可见。

**验证**：截图 4、15：查找对话框打开时，选中的 hello 仍是蓝底。

## 10. 截图工具：模态框、菜单动画与越界

**问题**：
- `QMessageBox::warning()` 会阻塞在 `exec()`，直接截图截不到；
- 菜单弹出有淡入动画，200 ms 时截出来是半透明的；
- 对话框被移到窗口外时，截图会把桌面背景带进来。

**修改**（`tools/shots/shots.cpp`）：
- 用 `QTimer::singleShot` 在模态框的嵌套事件循环里截图，再点“取消”/关闭；
- 菜单弹出后等 600 ms 再截；
- 对话框位置夹在父窗口内，截图只取所有可见顶层窗口的外接矩形；
- 截图前把鼠标移出窗口，避免工具栏按钮的悬停高亮。

**验证**：25 张截图重跑三次，尺寸稳定在 975×688，没有桌面背景。

## 11. QtTest 在 offscreen 平台下的两个坑

**问题一**：带弹窗的用例直接卡死。`-platform offscreen` 下窗口不会真正激活，
`QApplication::activeModalWidget()` 返回空，定时器关不掉消息框。

**修改**：改为遍历 `topLevelWidgets()`，找到可见的 `QMessageBox` 后关闭。

**问题二**：`findIsCaseSensitive` 断言失败，实际 11、期望 6。
把 `textCursor().positionInBlock()` 当成了选区起点，实际它是光标位置（选区末尾之后）。

**修改**：改用 `selectionStart()`。

**验证**：`tst_editor.exe -platform offscreen` → `Totals: 20 passed, 0 failed`。

## 12. 遗留问题

- 自动保存间隔固定 60 秒，没有做成可配置；
- 大文件（接近 32 MB）打开时是同步读入，界面会短暂卡顿；
- 没有多标签 / 最近文件列表；
- GBK 支持依赖 Windows 代码页，换到 Linux 需要另配编码表。
