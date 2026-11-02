#include "fileio.h"

#include <QByteArray>
#include <QFile>
#include <QSaveFile>
#include <QStringDecoder>
#include <QVector>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace FileIo {

const char kUtf8[]    = "UTF-8";
const char kUtf8Bom[] = "UTF-8 BOM";
const char kUtf16Le[] = "UTF-16 LE";
const char kUtf16Be[] = "UTF-16 BE";
const char kGbk[]     = "GBK";

namespace {

// ---- 字节流解码 ---------------------------------------------------------

bool decodeUtf8(const QByteArray &data, QString *text)
{
    QStringDecoder decoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    const QString result = decoder.decode(data);
    if (decoder.hasError())
        return false;
    *text = result;
    return true;
}

bool decodeUtf16(const QByteArray &raw, bool littleEndian, QString *text)
{
    if (raw.size() % 2 != 0)
        return false;

    QVector<ushort> units(raw.size() / 2);
    for (int i = 0; i < units.size(); ++i) {
        const uchar first  = static_cast<uchar>(raw.at(2 * i));
        const uchar second = static_cast<uchar>(raw.at(2 * i + 1));
        units[i] = littleEndian ? static_cast<ushort>(first | (second << 8))
                                : static_cast<ushort>(second | (first << 8));
    }
    *text = QString::fromUtf16(reinterpret_cast<const char16_t *>(units.constData()),
                               units.size());
    return true;
}

// GBK 的编解码借助 Windows 的代码页 936：MultiByteToWideChar 的
// MB_ERR_INVALID_CHARS 可以严格判断字节序列是否合法，正好满足“先试
// UTF-8，失败再退回 GBK”的需求。
bool decodeGbk(const QByteArray &data, bool strict, QString *text)
{
#ifdef Q_OS_WIN
    if (data.isEmpty()) {
        text->clear();
        return true;
    }
    const DWORD flags = strict ? MB_ERR_INVALID_CHARS : 0;
    const int length = ::MultiByteToWideChar(936, flags, data.constData(), data.size(),
                                             nullptr, 0);
    if (length <= 0)
        return false;

    QVector<wchar_t> buffer(length);
    const int written = ::MultiByteToWideChar(936, flags, data.constData(), data.size(),
                                              buffer.data(), length);
    if (written <= 0)
        return false;

    *text = QString::fromWCharArray(buffer.constData(), written);
    return true;
#else
    Q_UNUSED(data);
    Q_UNUSED(strict);
    Q_UNUSED(text);
    return false;   // 非 Windows 平台不支持 GBK
#endif
}

QByteArray encodeGbk(const QString &text, bool *ok)
{
#ifdef Q_OS_WIN
    if (text.isEmpty()) {
        *ok = true;
        return QByteArray();
    }
    const wchar_t *wide = reinterpret_cast<const wchar_t *>(text.utf16());
    const int wideLength = text.size();

    BOOL usedDefault = FALSE;
    const int length = ::WideCharToMultiByte(936, WC_NO_BEST_FIT_CHARS, wide, wideLength,
                                             nullptr, 0, nullptr, &usedDefault);
    if (length <= 0 || usedDefault) {
        *ok = false;
        return QByteArray();
    }

    QByteArray encoded(length, Qt::Uninitialized);
    usedDefault = FALSE;
    const int written = ::WideCharToMultiByte(936, WC_NO_BEST_FIT_CHARS, wide, wideLength,
                                              encoded.data(), length, nullptr, &usedDefault);
    if (written <= 0 || usedDefault) {
        *ok = false;
        return QByteArray();
    }
    encoded.resize(written);
    *ok = true;
    return encoded;
#else
    Q_UNUSED(text);
    *ok = false;
    return QByteArray();
#endif
}

} // namespace

QString normalizedEncoding(const QString &encoding)
{
    const QString key = encoding.trimmed().toUpper();
    if (key == QLatin1String("UTF-8 BOM") || key == QLatin1String("UTF8 BOM")
        || key == QLatin1String("UTF-8-BOM"))
        return QString::fromLatin1(kUtf8Bom);
    if (key == QLatin1String("UTF-16 LE") || key == QLatin1String("UTF16LE"))
        return QString::fromLatin1(kUtf16Le);
    if (key == QLatin1String("UTF-16 BE") || key == QLatin1String("UTF16BE"))
        return QString::fromLatin1(kUtf16Be);
    if (key == QLatin1String("GBK") || key == QLatin1String("GB18030")
        || key == QLatin1String("GB2312"))
        return QString::fromLatin1(kGbk);
    return QString::fromLatin1(kUtf8);
}

bool canEncodeText(const QString &text, const QString &encoding)
{
    const QString normalized = normalizedEncoding(encoding);

    if (normalized == QLatin1String(kGbk)) {
        bool ok = false;
        encodeGbk(text, &ok);
        return ok;
    }

    // UTF-16 与 UTF-8 都能表示任意 QString，不存在“无法表示”的字符。
    return true;
}

bool readTextFile(const QString &path, QString *text, QString *encoding, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        *error = file.errorString();
        return false;
    }
    file.close();

    // 1. 先看 BOM。UTF-16 数据本身含 0x00 字节，必须在查 NUL 之前排除。
    if (data.startsWith("\xEF\xBB\xBF")) {
        if (!decodeUtf8(data.mid(3), text)) {
            *error = QStringLiteral("UTF-8 BOM 文件的内容不是合法的 UTF-8。");
            return false;
        }
        *encoding = QString::fromLatin1(kUtf8Bom);
        return true;
    }
    if (data.startsWith("\xFF\xFE")) {
        if (!decodeUtf16(data.mid(2), true, text)) {
            *error = QStringLiteral("UTF-16 LE 文件的长度不是偶数。");
            return false;
        }
        *encoding = QString::fromLatin1(kUtf16Le);
        return true;
    }
    if (data.startsWith("\xFE\xFF")) {
        if (!decodeUtf16(data.mid(2), false, text)) {
            *error = QStringLiteral("UTF-16 BE 文件的长度不是偶数。");
            return false;
        }
        *encoding = QString::fromLatin1(kUtf16Be);
        return true;
    }

    // 2. 没有 BOM 的文本里不应该出现 NUL，出现了基本可以判定是二进制文件。
    if (data.contains('\0')) {
        *error = QStringLiteral("文件中含有 NUL 字节，看起来是二进制文件");
        return false;
    }

    // 3. 先严格试 UTF-8，成功即 UTF-8；失败再按 GBK 解释。
    if (decodeUtf8(data, text)) {
        *encoding = QString::fromLatin1(kUtf8);
        return true;
    }

    if (decodeGbk(data, true, text) || decodeGbk(data, false, text)) {
        *encoding = QString::fromLatin1(kGbk);
        return true;
    }

    *error = QStringLiteral("无法识别文件的编码（既不是 UTF-8 也不是 GBK）。");
    return false;
}

bool writeTextFile(const QString &path, const QString &text, const QString &encoding,
                   QString *error)
{
    const QString normalized = normalizedEncoding(encoding);

    QByteArray payload;
    bool ok = true;

    if (normalized == QLatin1String(kGbk)) {
        payload = encodeGbk(text, &ok);
    } else if (normalized == QLatin1String(kUtf16Le)
               || normalized == QLatin1String(kUtf16Be)) {
        payload.reserve(text.size() * 2 + 2);
        payload.append(normalized == QLatin1String(kUtf16Le) ? '\xFF' : '\xFE');
        payload.append(normalized == QLatin1String(kUtf16Le) ? '\xFE' : '\xFF');
        for (const QChar &ch : text) {
            const ushort code = ch.unicode();
            const char low = static_cast<char>(code & 0xFF);
            const char high = static_cast<char>((code >> 8) & 0xFF);
            if (normalized == QLatin1String(kUtf16Le)) {
                payload.append(low);
                payload.append(high);
            } else {
                payload.append(high);
                payload.append(low);
            }
        }
    } else {
        payload = text.toUtf8();
        if (normalized == QLatin1String(kUtf8Bom))
            payload.prepend("\xEF\xBB\xBF");
    }

    if (!ok) {
        *error = QStringLiteral("当前编码无法表示文档中的全部字符");
        return false;
    }

    // QSaveFile：先写同目录下的临时文件，commit() 时再原子替换目标文件，
    // 因此写失败不会把原文件截断成空文件。
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        *error = file.errorString();
        return false;
    }

    const qint64 written = file.write(payload);
    if (written != payload.size() || file.error() != QFileDevice::NoError) {
        *error = file.errorString();
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        *error = file.errorString();
        return false;
    }

    return true;
}

} // namespace FileIo
