import pathlib
import sys
import threading

# 使用项目内安装的第三方库，避免污染系统环境。
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent / ".vendor"))

import dns.exception
import dns.message
import dns.query
import dns.rcode
import dns.rdatatype


def query_once(qname: str) -> None:
    query = dns.message.make_query(qname, dns.rdatatype.A)

    try:
        response = dns.query.udp(query, "127.0.0.1", port=5533, timeout=3)
        print(
            f"{qname}: id={response.id}, "
            f"rcode={dns.rcode.to_text(response.rcode())}, "
            f"answer_count={len(response.answer)}"
        )
        for rrset in response.answer:
            print(f"{qname}: {rrset.to_text()}")
    except dns.exception.Timeout:
        print(f"{qname}: timeout")
    except Exception as exc:
        print(f"{qname}: error: {exc}")


def main() -> int:
    qnames = sys.argv[1:] if len(sys.argv) > 1 else [
        "www.y.com.cn",
        "008.cn",
        "www.baidu.com",
        "www.qq.com",
    ]
    threads = []

    for qname in qnames:
        thread = threading.Thread(target=query_once, args=(qname,))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
