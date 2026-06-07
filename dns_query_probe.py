import pathlib
import sys

# 使用项目内安装的第三方库，避免污染系统环境。
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent / ".vendor"))

import dns.exception
import dns.message
import dns.query
import dns.rdatatype


def main() -> int:
    query = dns.message.make_query("www.baidu.com", dns.rdatatype.A)

    try:
        response = dns.query.udp(query, "127.0.0.1", port=5533, timeout=2)
        print(f"response id={response.id}, answer_count={len(response.answer)}")
    except dns.exception.Timeout:
        print("timeout")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
