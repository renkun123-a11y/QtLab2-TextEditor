"""生成测试用的示例文本（UTF-8 / GBK / 二进制）。

用法：python tools/make_samples.py
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAMPLES = ROOT / "samples"

TEXT = """小牛文本编辑器 —— 测试文档
========================================

这是一段中文内容，用来验证编辑器对中文的显示、换行和选择操作。
The quick brown fox jumps over the lazy dog.

第 3 行：查找功能会在这行里找到 hello 这个词。
第 4 行：hello again, and hello once more.
第 5 行：最后一行，用来检查行号显示是否正确。
"""

LONG_LINE = (
    "这一行故意写得很长，用来验证自动换行打开时，一行文字被折成多行显示、"
    "但行号区仍然只占一个行号：abcdefghijklmnopqrstuvwxyz0123456789 "
    "abcdefghijklmnopqrstuvwxyz0123456789 abcdefghijklmnopqrstuvwxyz0123456789 "
    "最后再补一句中文，让折行跨过三行以上。"
)


def main():
    SAMPLES.mkdir(parents=True, exist_ok=True)

    (SAMPLES / "sample_utf8.txt").write_text(TEXT, encoding="utf-8", newline="\n")
    (SAMPLES / "sample_gbk.txt").write_text(TEXT, encoding="gbk", newline="\r\n")
    (SAMPLES / "sample_utf8_bom.txt").write_bytes(
        b"\xef\xbb\xbf" + TEXT.encode("utf-8")
    )
    (SAMPLES / "long_line.txt").write_text(TEXT + "\n" + LONG_LINE + "\n",
                                          encoding="utf-8", newline="\n")
    (SAMPLES / "readonly.txt").write_text(
        "这一行来自只读文件 readonly.txt。\n把文件属性设为只读后再保存，用来验证保存失败的处理。\n",
        encoding="utf-8", newline="\n")
    (SAMPLES / "fake_binary.dat").write_bytes(bytes(range(256)) * 2)

    print(f"samples written to {SAMPLES}")


if __name__ == "__main__":
    main()
