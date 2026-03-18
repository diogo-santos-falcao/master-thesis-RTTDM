import requests
import time
import subprocess
import json
import argparse
from datetime import datetime

# ==========================================
# CONFIGURAÇÕES DA REDE E SISTEMA
# ==========================================
TD3_BASE_URL = "http://172.25.16.1:8999"
TD3_API_CMD = f"{TD3_BASE_URL}/war/do"
TD3_API_POLL = f"{TD3_BASE_URL}/poll/?parts=status,trains"

C_PLUS_PLUS_EXECUTABLE = "./timetabler"

# ==========================================
# FUNÇÕES DE COMUNICAÇÃO COM O SIMULADOR
# ==========================================
def send_api_command(cmd):
    """Envia um comando HTTP para o Train Director 3"""
    try:
        url = f"{TD3_API_CMD}?{cmd.replace(' ', '%20')}"
        response = requests.get(url, timeout=2)
        if response.status_code == 200:
            print(f"    [COMANDO] '{cmd}' executado.")
            return True
        return False
    except requests.exceptions.RequestException as e:
        print(f"    [ERRO DE COMUNICAÇÃO] {e}")
        return False

def is_simulator_running():
    """Consulta o radar para saber se o simulador está em Play (1) ou Pause (0)"""
    try:
        response = requests.get(TD3_API_POLL, timeout=2)
        if response.status_code == 200:
            dados = response.json()
            return dados.get("status", {}).get("running", 0) == 1
    except:
        return False
    return False

def pause_simulation():
    print("\n>>> A TENTAR PAUSAR A SIMULAÇÃO...")
    if is_simulator_running():
        send_api_command("run") # O 'run' atua como Pause se estiver a andar!
        print("    [TMS] Simulação pausada com sucesso.")
    else:
        print("    [TMS] A simulação JÁ ESTAVA pausada.")

def resume_simulation():
    print("\n>>> A TENTAR RETOMAR A SIMULAÇÃO...")
    if not is_simulator_running():
        send_api_command("run") # O 'run' atua como Play se estiver parada!
        print("    [TMS] Simulação retomada com sucesso.")
    else:
        print("    [TMS] A simulação JÁ ESTAVA a correr.")

def inject_delay(train_id, delay_minutes):
    """Injeta um atraso num comboio específico via API"""
    print(f"\n>>> A INJETAR DISRUPÇÃO: Atrasar o comboio {train_id} em {delay_minutes} minutos.")
    send_api_command(f"delay {train_id} {delay_minutes}")

def get_original_schedules_from_server():
    """Lê o radar no início para mapear e guardar os horários originais de partida"""
    print("\n>>> A EXTRAIR HORÁRIOS ORIGINAIS DO SIMULADOR...")
    horarios_extraidos = {}
    try:
        resposta = requests.get(TD3_API_POLL, timeout=2)
        if resposta.status_code == 200:
            comboios = resposta.json().get("trains", [])
            for c in comboios:
                nome = c.get("name")
                hora_inicio = c.get("timeIn", "").strip() 
                
                # Garante o formato HH:MM:SS para a nossa matemática
                if len(hora_inicio) == 7:
                    hora_inicio = "0" + hora_inicio
                    
                if nome and hora_inicio:
                    horarios_extraidos[nome] = hora_inicio
                    
            print(f"    [TMS] Guardados os horários base de {len(horarios_extraidos)} comboios com sucesso!")
            return horarios_extraidos
    except Exception as e:
        print(f"    [ERRO] Falha ao comunicar com o radar: {e}")
        
    return horarios_extraidos

# ==========================================
# FUNÇÕES DO MOTOR DE OTIMIZAÇÃO (C++)
# ==========================================
def run_solver(trk_file, sch_file, comboio_avaria, atraso_minutos):
    """Chama o executável C++ passando-lhe os ficheiros e a disrupção!"""
    print("\n>>> A INVOCAR O CÉREBRO DE OTIMIZAÇÃO (MaxSAT)...")
    
    # O C++ pede o atraso em segundos, portanto multiplicamos por 60
    atraso_segundos = atraso_minutos * 60
    
    command = [
        C_PLUS_PLUS_EXECUTABLE,
        f"-td3-trk={trk_file}",
        f"-td3-sch={sch_file}",
        f"-disrupt-train={comboio_avaria}",
        f"-disrupt-time={atraso_segundos}",
        "-opt-time=2"
    ]
    
    start_time = time.time()
    try:
        process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if process.returncode != 0:
            print(f"[ERRO NO C++] {process.stderr}")
            return False
        
        print(f">>> Otimização concluída com sucesso em {time.time() - start_time:.2f} segundos!")
        return True
    except FileNotFoundError:
        print(f"[ERRO] Executável '{C_PLUS_PLUS_EXECUTABLE}' não encontrado.")
        return False

# ==========================================
# APLICAÇÃO DA SOLUÇÃO OTIMIZADA
# ==========================================
def time_diff_seconds(time1_str, time2_str):
    """Calcula a diferença em segundos entre duas strings de horas (HH:MM:SS)"""
    fmt = '%H:%M:%S'
    t1 = datetime.strptime(time1_str, fmt)
    t2 = datetime.strptime(time2_str, fmt)
    if t2 < t1:
        return (t2 - t1).total_seconds() + 86400
    return (t2 - t1).total_seconds()

def apply_optimized_solution(json_file_path, horarios_originais):
    """Lê o JSON do C++ e injeta os atrasos/esperas calculados no TD3"""
    print(f"\n>>> A PROCESSAR A SOLUÇÃO OTIMIZADA ({json_file_path})...")
    
    try:
        with open(json_file_path, 'r') as file:
            dados = json.load(file)
            
        comboios_alterados = 0
        
        # Percorre a lista de comboios gerada pelo teu MaxSAT
        for comboio in dados.get("train_runs", []):
            train_id = comboio.get("service_intention_id")
            seccoes = comboio.get("train_run_sections", [])
            
            if not seccoes:
                continue
                
            # Verifica a hora da primeira secção otimizada
            primeira_seccao = seccoes[0]
            hora_partida_otimizada = primeira_seccao.get("entry_time")
            
            # Compara com o horário original que o Python extraiu no início
            hora_partida_original = horarios_originais.get(train_id)
            
            if hora_partida_original and hora_partida_otimizada:
                atraso_segundos = time_diff_seconds(hora_partida_original, hora_partida_otimizada)
                
                # Se o solver decidiu atrasar o comboio para evitar colisões
                if atraso_segundos > 0:
                    atraso_minutos = int(atraso_segundos / 60)
                    print(f"    [CURA] O Solver mandou atrasar o comboio {train_id} em {atraso_minutos} min.")
                    
                    # Envia a ordem para o simulador
                    send_api_command(f"delay {train_id} {atraso_minutos}")
                    comboios_alterados += 1
                    
        print(f">>> Resumo: {comboios_alterados} comboios foram reescalonados para evitar colisões.")
        return True

    except Exception as e:
        print(f"[ERRO CRÍTICO] Falha ao ler JSON ou aplicar solução: {e}")
        return False

# ==========================================
# CICLO DE VIDA PRINCIPAL (WORKFLOW)
# ==========================================
if __name__ == "__main__":
    # Configuração dos Argumentos do Terminal (Torna o script universal!)
    parser = argparse.ArgumentParser(description="Traffic Management System (TMS) - Dissertação")
    parser.add_argument("--trk", required=True, help="Caminho para o ficheiro .trk do cenário")
    parser.add_argument("--sch", required=True, help="Caminho para o ficheiro .sch do cenário")
    parser.add_argument("--out", required=True, help="Nome do ficheiro .json gerado pelo C++")
    parser.add_argument("--comboio", required=True, help="Nome do comboio que vai sofrer a avaria")
    parser.add_argument("--atraso", type=int, default=15, help="Tempo de atraso em minutos (default: 15)")
    
    args = parser.parse_args()

    print(f"=== TMS INICIADO: Cenário {args.sch} ===\n")
    
    # 1. Garante que a simulação está a andar
    resume_simulation()
    
    # 2. O Python descobre os horários originais sozinho pelo Radar
    horarios_originais = get_original_schedules_from_server()
    
    print(">>> A observar a rede a funcionar normalmente...")
    time.sleep(5) # Espera 5 segundos para tu vires o jogo a correr
    
    # 3. Acontece a disrupção no comboio que escolheste no terminal!
    pause_simulation()
    inject_delay(args.comboio, args.atraso) 
    
    # 4. Chamar o C++ (Passando as variáveis do terminal)
    if run_solver(args.trk, args.sch, args.comboio, args.atraso):
        
        # 5. Ler o JSON otimizado e aplicar a cura
        apply_optimized_solution(args.out, horarios_originais)
        
        # 6. Retomar a circulação curada
        print("\n>>> Sistema estabilizado. A retomar a circulação em 3 segundos...")
        time.sleep(3)
        resume_simulation()
    else:
        print(">>> [ALERTA] Falha na otimização. Não foi possível retomar a operação.")
        
    print("\n=== TMS EM MODO STANDBY ===")