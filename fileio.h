#ifndef FILEIO_H
#define FILEIO_H

#include <QString>

// 文本文件读写与编码识别。
//
// 编辑器自身不关心字节，只处理 QString；这个命名空间负责三件事：
//   1. 打开时识别编码（BOM -> 严格 UTF-8 -> GBK），并拒绝二进制文件；
//   2. 保存时按原来的编码写回，写之前先确认每个字符都能被该编码表示；
//   3. 用 QSaveFile 做原子写入，失败时文件内容保持不变。
namespace FileIo {

extern const char kUtf8[];      // "UTF-8"
extern const char kUtf8Bom[];   // "UTF-8 BOM"
extern const char kUtf16Le[];   // "UTF-16 LE"
extern const char kUtf16Be[];   // "UTF-16 BE"
extern const char kGbk[];       // "GBK"

// 文本中每个字符都能被 encoding 表示时返回 true。
bool canEncodeText(const QString &text, const QString &encoding);

// 读取文本文件。
// 成功时写入 text 与识别出的 encoding 并返回 true；
// 失败时写入可读的错误说明并返回 false。
bool readTextFile(const QString &path, QString *text, QString *encoding, QString *error);

// 按指定编码原子写入文本文件；失败时通过 error 返回原因。
bool writeTextFile(const QString &path, const QString &text, const QString &encoding, QString *error);

// 把编码名规范化（大小写、常见别名），未知编码按 UTF-8 处理。
QString normalizedEncoding(const QString &encoding);

} // namespace FileIo

#endif // FILEIO_H
