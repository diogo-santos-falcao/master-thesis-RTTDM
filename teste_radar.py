import requests
import json

# Usamos o teu IP mágico e o URL de polling que descobriste!
URL_RADAR = "http://172.25.16.1:8999/poll/?parts=status,trains"

print(">>> A ligar ao Radar do TD3...")
try:
    resposta = requests.get(URL_RADAR, timeout=3)
    
    if resposta.status_code == 200:
        dados = resposta.json() # Converte a resposta para um Dicionário Python
        
        # 1. Imprimir a secção de STATUS
        print("\n--- STATUS DO SIMULADOR ---")
        status_info = dados.get("status", {})
        print(json.dumps(status_info, indent=2))
        
        # 2. Imprimir a secção dos COMBOIOS
        comboios = dados.get("trains", [])
        print(f"\n--- COMBOIOS DETETADOS: {len(comboios)} ---")
        
        if len(comboios) > 0:
            print("Dados do primeiro comboio da lista:")
            # Imprime os detalhes do primeiro comboio de forma bonita
            print(json.dumps(comboios[0], indent=2)) 
            
    else:
        print(f"Erro na resposta: {resposta.status_code}")
        
except Exception as e:
    print(f"ERRO de ligação: {e}")