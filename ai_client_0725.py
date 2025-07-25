#!/usr/bin/env python3
# coding: utf-8

import os, sys, socket, json
from openai import OpenAI

api_key = "aa"
client = OpenAI(api_key=api_key)

port_num = 10006

def chat_with_gpt35(messages: list) -> str:
    resp = client.chat.completions.create(
        model="gpt-3.5-turbo",
        messages=messages,
        temperature=0.3,
        max_tokens=300,
    )
    return resp.choices[0].message.content.strip()

def run_tcp_client(host: str = "127.0.0.1", port: int = port_num):
    print(f"🔗 서버에 연결 시도 → {host}:{port}", flush=True)
    try:
        with socket.create_connection((host, port)) as sock, sock.makefile('rwb') as f:
            print(f"✅ 연결 성공 → {host}:{port}", flush=True)

            def init_messages():
                return [{
                    "role": "system",
                    "content": (
                        "너는 질병 증상 상담 전용 AI야. 사용자의 증상을 최대 10번 이내의 질문으로 파악해.\n"
                        "질문은 반드시 자연스럽고, 간결한 대화 형식으로 해. 숫자나 목차를 붙이지 말고 꼭 '일상 대화하듯' 부드럽게 질문해.\n"
                        "예: '통증이 시작된 건 언제인가요?' 또는 '어디가 아프신가요?'처럼 친절하고 짧게 묻기.\n"
                        "질문은 한 번에 하나씩, 짧고 쉬운 말로!\n\n"
                        "상담 마지막에는 반드시 아래 두 가지 정보를 함께 제시해:\n"
                        "완화 방안: 사용자가 증상을 완화하기 위해 스스로 시도할 수 있는 쉬운 방법을 알려줘.\n"
                        "추천 진료과: **진료과 이름만 쉼표로 구분해서 나열해. 설명이나 수식어 없이 과 이름만 출력해. 예: 정형외과, 내과**\n\n"
                        "완화 방안:\n\n추천 진료과:\n\n"
                        "만약 사용자의 입력이 의료 증상과 관련이 없거나, 잡담/농담/기타 요청일 경우에는\n"
                        "'질병 증상 상담 목적의 질문을 입력해 주세요.' 또는 '의료와 관련된 증상을 알려주세요.'라고 안내해줘."
                    )
                }]

            messages = init_messages()
            question_count = 0
            MAX_QUESTIONS = 10
            consultation_done = False

            while True:
                raw = f.readline()
                if not raw:
                    print("🔌 서버 종료", flush=True)
                    break

                line = raw.decode('utf-8').strip()
                if not line:
                    continue

                try:
                    req = json.loads(line)
                except json.JSONDecodeError as e:
                    print("⚠️ JSON 오류:", e, flush=True)
                    continue

                prompt = req.get("prompt")
                if not prompt:
                    continue

                print(f"📨 사용자 입력({question_count + 1}회차): {prompt}", flush=True)

                # 상담 재시작 명령어
                if prompt.strip() == "상담 재시작":
                    messages = init_messages()
                    question_count = 0
                    consultation_done = False
                    continue

                # 상담 종료 후에는 응답하지 않음
                if consultation_done:
                    print("🔇 상담 종료 이후 입력 무시됨", flush=True)
                    continue

                # GPT 처리
                messages.append({"role": "user", "content": prompt})

                try:
                    reply = chat_with_gpt35(messages)
                    messages.append({"role": "assistant", "content": reply})
                    question_count += 1

                    if "완화 방안" in reply and "추천 진료과" in reply:
                        consultation_done = True

                        # 첫 번째 응답: AI 답변만
                        resp_obj = {"signal": "ai_answer", "result": reply}
                        out = json.dumps(resp_obj, ensure_ascii=False) + "\n"
                        f.write(out.encode('utf-8'))
                        f.flush()
                        print("✅ GPT 응답 전송:", resp_obj, flush=True)

                        # 두 번째 응답: 종료 안내 메시지
                        end_obj = {
                            "signal": "ai_answer",
                            "result": "상담이 종료되었습니다. 더 이상 질문을 받을 수 없습니다."
                        }
                        out = json.dumps(end_obj, ensure_ascii=False) + "\n"
                        f.write(out.encode('utf-8'))
                        f.flush()
                        print("✅ 종료 안내 전송:", end_obj, flush=True)
                        continue  # 다음 루프 건너뜀

                    resp_obj = {"signal": "ai_answer", "result": reply}

                except Exception as e:
                    resp_obj = {"signal": "ai_answer", "error": str(e)}

                out = json.dumps(resp_obj, ensure_ascii=False) + "\n"
                f.write(out.encode('utf-8'))
                f.flush()
                print("✅ GPT 응답 전송:", resp_obj, flush=True)

    except Exception as e:
        print("❌ 오류:", e, flush=True)

if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) >= 2 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) >= 3 else port_num
    run_tcp_client(host, port)

