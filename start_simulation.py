import requests
import time

URL_COMANDOS = "http://172.25.16.1:8999/war/do"

def enviar(comando):
    print(f"> A enviar ordem: '{comando}'...")
    try:
        resposta = requests.get(f"{URL_COMANDOS}?{comando}", timeout=3)
        print(f"  Sucesso! Código: {resposta.status_code}")
    except Exception as erro:
        print(f"  ERRO de ligação: {erro}")

print("=== TESTE DE CONTROLO REMOTO TD3 ===")

enviar("run")

print("=== TESTE CONCLUÍDO ===")