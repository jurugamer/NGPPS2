#!/usr/bin/env python3
"""
╔══════════════════════════════════════════════════════════════════════╗
║     PS2 Launcher Studio — Tema & Conversor TIM2 Oficial Sony         ║
║     App Desktop com UI Web via pywebview + Exportador TIM2 Nativo     ║
╚══════════════════════════════════════════════════════════════════════╝
"""

import os
import sys
import io
import math
import base64
import struct
import zipfile
import subprocess
import threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from PIL import Image
import webview

# ══════════════════════════════════════════════════════════════════════════════
#  MOTOR CONVERSOR TIM2 DO PLAYSTATION 2 (GS NATIVE)
# ══════════════════════════════════════════════════════════════════════════════

TIM2_MAGIC   = b'TIM2'
TIM2_VERSION = 0x04
TIM2_ALIGN   = 128

IMG_RGBA16   = 0x01
IMG_RGBA32   = 0x03
CLUT_NONE    = 0x00
GS_PSM_CT32  = 0x00
GS_PSM_CT16  = 0x02

BAYER_4X4 = [
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
]
DITHER_TABLE = [(val / 16.0 - 0.5) * 8.226 for val in BAYER_4X4]


def align_up(size: int, alignment: int) -> int:
    return (size + alignment - 1) & ~(alignment - 1)


def ps2_alpha(a: int) -> int:
    return round(a * 128 / 255)


def premultiply_alpha(img: Image.Image) -> Image.Image:
    """Pré-multiplicação de Alpha para evitar bordas escuras nos quads e sprites do PS2."""
    img = img.convert('RGBA')
    r, g, b, a = img.split()
    a_vals = list(a.getdata())
    def _mul_channel(ch: Image.Image) -> Image.Image:
        ch_vals = list(ch.getdata())
        result = [round(c * av / 255) for c, av in zip(ch_vals, a_vals)]
        out = Image.new('L', img.size)
        out.putdata(result)
        return out
    return Image.merge('RGBA', (_mul_channel(r), _mul_channel(g), _mul_channel(b), a))


def compute_gs_tex0(width: int, height: int, psm: int) -> int:
    tbw = max(1, align_up(width, 64) // 64)
    tw = max(0, math.ceil(math.log2(max(width, 1))))
    th = max(0, math.ceil(math.log2(max(height, 1))))
    v = 0
    v |= (tbw & 0x3F) << 14
    v |= (psm & 0x3F) << 20
    v |= (tw  & 0xF)  << 26
    v |= (th  & 0xF)  << 30
    v |= (1   & 0x1)  << 34
    v |= (0   & 0xF)  << 51
    v |= (1   & 0x7)  << 61
    return v & 0xFFFFFFFFFFFFFFFF


def build_picture_block(pixel_data: bytes, width: int, height: int, img_type: int, psm: int) -> bytes:
    HEADER = 48
    raw_size = HEADER + len(pixel_data)
    total_size = align_up(raw_size, TIM2_ALIGN)
    padding = total_size - raw_size
    gs_tex0 = compute_gs_tex0(width, height, psm)

    hdr = bytearray(HEADER)
    struct.pack_into('<I', hdr,  0, total_size)
    struct.pack_into('<I', hdr,  4, 0)
    struct.pack_into('<I', hdr,  8, len(pixel_data))
    struct.pack_into('<H', hdr, 12, HEADER)
    struct.pack_into('<H', hdr, 14, 0)
    hdr[16] = 0
    hdr[17] = 1
    hdr[18] = CLUT_NONE
    hdr[19] = img_type
    struct.pack_into('<H', hdr, 20, width)
    struct.pack_into('<H', hdr, 22, height)
    struct.pack_into('<Q', hdr, 24, gs_tex0)
    struct.pack_into('<I', hdr, 40, 0)

    return bytes(hdr) + pixel_data + (b'\x00' * padding)


def convert_to_tim2_32bit(img: Image.Image, premult: bool = True) -> bytes:
    """TIM2 32-bit RGBA com Alpha Opcional."""
    if premult:
        img = premultiply_alpha(img)
    img = img.convert('RGBA')
    w, h = img.size
    raw = img.tobytes()

    data = bytearray(len(raw))
    for i in range(0, len(raw), 4):
        data[i]     = raw[i]
        data[i + 1] = raw[i + 1]
        data[i + 2] = raw[i + 2]
        data[i + 3] = ps2_alpha(raw[i + 3])

    block = build_picture_block(bytes(data), w, h, IMG_RGBA32, GS_PSM_CT32)
    hdr = struct.pack('<4sBBH8s', TIM2_MAGIC, TIM2_VERSION, 0x00, 1, b'\x00' * 8)
    return hdr + block


def convert_to_tim2_16bit(img: Image.Image, premult: bool = False, dither: bool = True) -> bytes:
    """TIM2 16-bit (RGB5551) com dithering Bayer 4x4."""
    if premult:
        img = premultiply_alpha(img)
    img = img.convert('RGBA')
    w, h = img.size
    pixels = list(img.getdata())

    out = bytearray(w * h * 2)
    idx = 0
    for y in range(h):
        y_mod = (y & 3) << 2
        for x in range(w):
            r, g, b, a = pixels[idx]
            if dither:
                d = DITHER_TABLE[y_mod | (x & 3)]
                r5 = min(31, max(0, int(round((r + d) * 31.0 / 255.0))))
                g5 = min(31, max(0, int(round((g + d) * 31.0 / 255.0))))
                b5 = min(31, max(0, int(round((b + d) * 31.0 / 255.0))))
            else:
                r5 = min(31, max(0, round(r * 31.0 / 255.0)))
                g5 = min(31, max(0, round(g * 31.0 / 255.0)))
                b5 = min(31, max(0, round(b * 31.0 / 255.0)))

            a1 = 1 if a >= 128 else 0
            val = (a1 << 15) | ((b5 & 0x1F) << 10) | ((g5 & 0x1F) << 5) | (r5 & 0x1F)
            struct.pack_into('<H', out, idx * 2, val)
            idx += 1

    block = build_picture_block(bytes(out), w, h, IMG_RGBA16, GS_PSM_CT16)
    hdr = struct.pack('<4sBBH8s', TIM2_MAGIC, TIM2_VERSION, 0x00, 1, b'\x00' * 8)
    return hdr + block


# ══════════════════════════════════════════════════════════════════════════════
#  ANALISADOR INTELIGENTE DE REGRAS (NOME DO ARQUIVO -> FORMATO TIM2)
# ══════════════════════════════════════════════════════════════════════════════

def process_image_to_tm2(filename: str, img: Image.Image) -> bytes:
    """Aplica as regras de conversão baseadas no nome do arquivo PNG fornecido."""
    name_upper = filename.upper()
    
    if "32BA" in name_upper:
        return convert_to_tim2_32bit(img, premult=True)
    elif "32B" in name_upper:
        return convert_to_tim2_32bit(img, premult=False)
    elif "16BA" in name_upper:
        return convert_to_tim2_16bit(img, premult=True, dither=True)
    elif "16B" in name_upper:
        return convert_to_tim2_16bit(img, premult=False, dither=True)
    elif "ATLAS" in name_upper:
        return convert_to_tim2_32bit(img, premult=True)
    elif "BACKGROUND" in name_upper:
        return convert_to_tim2_16bit(img, premult=True, dither=True)
    else:
        return convert_to_tim2_32bit(img, premult=False)


# ══════════════════════════════════════════════════════════════════════════════
#  ARQUIVOS DE CONFIGURAÇÃO E TRADUÇÃO
# ══════════════════════════════════════════════════════════════════════════════

CONFIG_LAUNCHER_CONTENT = """[features]
show_logo = 1
show_numbers = 0
show_subtitles = 1
show_tags = 1
show_scrollbar = 1
custom_background = 1
confirm_delete = 1
language = pt_br
exit_path = mass:/APPS/BOOT/OPL/BOOT.ELF

[messages]
empty_message = NENHUM JOGO ENCONTRADO
empty_subtitle = PRESSIONE START PARA RECARREGAR
"""

EN_US_LANG = """BTN_SELECT=Select
BTN_OPTIONS=Options
BTN_SETTINGS=Settings
BTN_BACK=Back
BTN_EXIT=Exit
BTN_DELETE=Delete
OPT_SHOW_LOGO=Show Logo
OPT_SHOW_LOGO_SUB=Theme logo at the top
OPT_SHOW_NUM=Game Numbers
OPT_SHOW_NUM_SUB=01., 02. numbers before title
OPT_SHOW_SUB=Game Subtitles
OPT_SHOW_SUB_SUB=Extra details and info
OPT_SHOW_TAGS=Badges and Serials
OPT_SHOW_TAGS_SUB=SLUS badges and tags box
OPT_SHOW_SCROLL=Scrollbar
OPT_SHOW_SCROLL_SUB=Side navigation indicator
OPT_CUSTOM_BG=Game Background
OPT_CUSTOM_BG_SUB=Load custom background.tm2
OPT_CONFIRM_DEL=Confirm Deletion
OPT_CONFIRM_DEL_SUB=Show warning dialog before deleting
OPT_LANG=Language
OPT_LANG_SUB=Select user interface language
OPT_EXIT_PATH=Exit Path
OPT_EXIT_PATH_SUB=Executable launched upon exit
GAME_TITLE=Edit Title
GAME_SUBTITLE=Edit Subtitle
GAME_TAG=Edit Tag
GAME_DEL_BG=Delete Background
GAME_DEL_BG_SUB=Remove custom background.tm2
PAUSE_CONTINUE=Continue Game
PAUSE_CONTINUE_SUB=Resume current gameplay
PAUSE_SAVE_STATE=Save State (Slot)
PAUSE_SAVE_STATE_SUB=Save progress in one of 9 slots
PAUSE_LOAD_STATE=Load State (Slot)
PAUSE_LOAD_STATE_SUB=Restore previous saved state
PAUSE_SAVE_BG=Save as Background
PAUSE_SAVE_BG_SUB=Use current screen as menu background
PAUSE_DEL_SAVE=Delete Savegame (.ngf)
PAUSE_DEL_SAVE_SUB=Clear battery save file
PAUSE_EXIT=Exit to Menu
PAUSE_EXIT_SUB=End game and return to launcher
CONFIRM_YES=Yes
CONFIRM_YES_SUB=Permanently delete file
CONFIRM_NO=No
CONFIRM_NO_SUB=Cancel and keep file
CONFIRM_DONT_ASK=Yes, don't ask again
CONFIRM_DONT_ASK_SUB=Delete and disable future warnings
SLOT_EMPTY=Empty
SLOT_SAVED=Saved
LANG_PT=Portugues (Brasil)
LANG_PT_SUB=Brazilian Portuguese
LANG_EN=English (US)
LANG_EN_SUB=Default English language
LANG_ES=Espanol
LANG_ES_SUB=Spanish translation
PAUSE_RESET=Reset Game
PAUSE_RESET_SUB=Reset core and restart from beginning
"""

ES_ES_LANG = """BTN_SELECT=Seleccionar
BTN_OPTIONS=Opciones
BTN_SETTINGS=Configuracion
BTN_BACK=Volver
BTN_EXIT=Salir
BTN_DELETE=Eliminar
OPT_SHOW_LOGO=Mostrar Logo
OPT_SHOW_LOGO_SUB=Logo del tema en la parte superior
OPT_SHOW_NUM=Numeracion de Juegos
OPT_SHOW_NUM_SUB=Numeros 01., 02. antes del titulo
OPT_SHOW_SUB=Subtitulos de Juegos
OPT_SHOW_SUB_SUB=Detalles e informacion extra
OPT_SHOW_TAGS=Insignias y Seriales
OPT_SHOW_TAGS_SUB=Caja lateral SLUS y etiquetas
OPT_SHOW_SCROLL=Barra de Desplazamiento
OPT_SHOW_SCROLL_SUB=Indicador lateral de navegacion
OPT_CUSTOM_BG=Fondo por Juego
OPT_CUSTOM_BG_SUB=Cargar background.tm2 individual
OPT_CONFIRM_DEL=Confirmar Eliminacion
OPT_CONFIRM_DEL_SUB=Mostrar advertencia antes de borrar
OPT_LANG=Idioma
OPT_LANG_SUB=Seleccionar idioma de la interfaz
OPT_EXIT_PATH=Ruta de Salida
OPT_EXIT_PATH_SUB=Ejecutable iniciado al salir
GAME_TITLE=Editar Titulo
GAME_SUBTITLE=Editar Subtitulo
GAME_TAG=Editar Etiqueta
GAME_DEL_BG=Eliminar Fondo
GAME_DEL_BG_SUB=Eliminar background.tm2 de este juego
PAUSE_CONTINUE=Continuar Juego
PAUSE_CONTINUE_SUB=Reanudar la partida actual
PAUSE_SAVE_STATE=Guardar Estado (Slot)
PAUSE_SAVE_STATE_SUB=Guardar progreso en uno de los 9 slots
PAUSE_LOAD_STATE=Cargar Estado (Slot)
PAUSE_LOAD_STATE_SUB=Recuperar estado guardado
PAUSE_SAVE_BG=Guardar como Fondo
PAUSE_SAVE_BG_SUB=Usar pantalla actual como fondo
PAUSE_DEL_SAVE=Borrar Partida (.ngf)
PAUSE_DEL_SAVE_SUB=Limpiar archivo de guardado de bateria
PAUSE_EXIT=Salir al Menu
PAUSE_EXIT_SUB=Terminar juego y volver al launcher
CONFIRM_YES=Si
CONFIRM_YES_SUB=Confirmar eliminacion definitiva
CONFIRM_NO=No
CONFIRM_NO_SUB=Cancelar y conservar archivo
CONFIRM_DONT_ASK=Si, no volver a preguntar
CONFIRM_DONT_ASK_SUB=Eliminar y desactivar avisos futuros
SLOT_EMPTY=Vacio
SLOT_SAVED=Guardado
LANG_PT=Portugues (Brasil)
LANG_PT_SUB=Idioma portugues de Brasil
LANG_EN=English (US)
LANG_EN_SUB=Traduccion al ingles
LANG_ES=Espanol
LANG_ES_SUB=Idioma espanol por defecto
PAUSE_RESET=Reiniciar Juego
PAUSE_RESET_SUB=Reinicia la consola y reinicia la partida
"""

PT_BR_LANG = """BTN_SELECT=Selecionar
BTN_OPTIONS=Opcoes
BTN_SETTINGS=Configuracoes
BTN_BACK=Voltar
BTN_EXIT=Sair
BTN_DELETE=Excluir
OPT_SHOW_LOGO=Exibir Logo
OPT_SHOW_LOGO_SUB=Logo do tema no topo da tela
OPT_SHOW_NUM=Numeracao dos Jogos
OPT_SHOW_NUM_SUB=Numeros 01., 02. antes do titulo
OPT_SHOW_SUB=Subtitulos dos Jogos
OPT_SHOW_SUB_SUB=Detalhes e informacoes extras
OPT_SHOW_TAGS=Badges e Seriais
OPT_SHOW_TAGS_SUB=Caixinha lateral SLUS e tags
OPT_SHOW_SCROLL=Barra de Rolagem
OPT_SHOW_SCROLL_SUB=Indicador lateral de navegacao
OPT_CUSTOM_BG=Fundo por Jogo
OPT_CUSTOM_BG_SUB=Carrega background.tm2 individual
OPT_CONFIRM_DEL=Confirmar Exclusao
OPT_CONFIRM_DEL_SUB=Exibir janela de aviso antes de apagar
OPT_LANG=Idioma
OPT_LANG_SUB=Selecione o idioma da interface
OPT_EXIT_PATH=Caminho de Saida
OPT_EXIT_PATH_SUB=Executavel disparado ao sair
GAME_TITLE=Editar Titulo
GAME_SUBTITLE=Editar Subtitulo
GAME_TAG=Editar Tag
GAME_DEL_BG=Excluir Fundo
GAME_DEL_BG_SUB=Remove background.tm2 deste jogo
PAUSE_CONTINUE=Continuar Jogo
PAUSE_CONTINUE_SUB=Retomar a partida atual
PAUSE_SAVE_STATE=Salvar Estado (Slot)
PAUSE_SAVE_STATE_SUB=Gravar progresso em um dos 9 slots
PAUSE_LOAD_STATE=Carregar Estado (Slot)
PAUSE_LOAD_STATE_SUB=Recuperar estado gravado
PAUSE_SAVE_BG=Salvar como Fundo
PAUSE_SAVE_BG_SUB=Usar a tela atual como fundo do menu
PAUSE_DEL_SAVE=Apagar Savegame (.ngf)
PAUSE_DEL_SAVE_SUB=Limpar arquivo de save da bateria
PAUSE_EXIT=Sair para o Menu
PAUSE_EXIT_SUB=Encerrar jogo e voltar ao launcher
CONFIRM_YES=Sim
CONFIRM_YES_SUB=Confirmar exclusao definitiva
CONFIRM_NO=Nao
CONFIRM_NO_SUB=Cancelar e manter o arquivo
CONFIRM_DONT_ASK=Sim, nao perguntar novamente
CONFIRM_DONT_ASK_SUB=Excluir e desativar avisos futuros
SLOT_EMPTY=Vazio
SLOT_SAVED=Gravado
LANG_PT=Portugues (Brasil)
LANG_PT_SUB=Idioma padrao do sistema
LANG_EN=English (US)
LANG_EN_SUB=English translation
LANG_ES=Espanol
LANG_ES_SUB=Traduccion al espanol
PAUSE_RESET=Reiniciar Jogo
PAUSE_RESET_SUB=Reinicia o console e recomeca a partida
"""

# ══════════════════════════════════════════════════════════════════════════════
#  API NATIVA EXPOSTA PARA A INTERFACE REACT (PYWEBVIEW API)
# ══════════════════════════════════════════════════════════════════════════════

def open_file_in_system(filepath: str):
    """Abre a pasta contendo o ZIP gerado no Explorer/Windows."""
    try:
        folder = os.path.dirname(filepath)
        if sys.platform == 'win32':
            os.startfile(folder)
        elif sys.platform == 'darwin':
            subprocess.run(['open', folder], check=False)
        else:
            subprocess.run(['xdg-open', folder], check=False)
    except Exception as e:
        pass


class StudioAPI:
    def export_theme(self, payload: dict):
        """
        1. Abre a janela nativa do Windows perguntando onde salvar o ZIP.
        2. Converte as imagens recebidas do Javascript.
        3. Compacta tudo no ZIP escolhido.
        """
        try:
            # Obter a janela ativa para chamar o File Dialog de salvar nativo
            window = webview.windows[0]
            
            # Pede ao usuário o local para salvar o arquivo (Sugere Área de Trabalho apenas como inicio, não obriga)
            save_path_tuple = window.create_file_dialog(
                webview.SAVE_DIALOG, 
                directory=os.path.expanduser("~\\Desktop") if sys.platform == 'win32' else "", 
                save_filename='ps2_launcher_assets.zip'
            )
            
            # Se o usuário cancelar a janela
            if not save_path_tuple or len(save_path_tuple) == 0:
                return {"success": False, "message": "Operação cancelada pelo usuário."}
                
            output_zip = save_path_tuple[0]

            # Montagem do ZIP final
            with zipfile.ZipFile(output_zip, 'w', zipfile.ZIP_DEFLATED) as zipf:

                # Processa dinamicamente todas as imagens enviadas no payload
                imgs = payload.get("images", {})
                for img_key, img_data in imgs.items():
                    filename = img_data.get("filename", f"{img_key}.png")
                    b64_data = img_data.get("base64", "")
                    
                    if not b64_data:
                        continue
                    
                    img = Image.open(io.BytesIO(base64.b64decode(b64_data)))
                    
                    # Converte para TIM2 seguindo as regras de nomenclatura
                    tm2_bytes = process_image_to_tm2(filename, img)
                    
                    # Altera a extensão final no pacote ZIP de .png para .tm2
                    tm2_filename = os.path.splitext(filename)[0] + ".tm2"
                    zipf.writestr(f"assets/{tm2_filename}", tm2_bytes)

                # Configurações do Tema e Launcher
                theme_cfg = payload.get("config_theme", {}).get("text", "")
                if theme_cfg:
                    zipf.writestr("assets/config_theme.cfg", theme_cfg.encode("utf-8"))
                
                zipf.writestr("assets/config_launcher.cfg", CONFIG_LAUNCHER_CONTENT.encode("utf-8"))

                # Idiomas na subpasta assets/translate/
                zipf.writestr("assets/translate/en_us.lang", EN_US_LANG.encode("utf-8"))
                zipf.writestr("assets/translate/es_es.lang", ES_ES_LANG.encode("utf-8"))
                zipf.writestr("assets/translate/pt_br.lang", PT_BR_LANG.encode("utf-8"))

            # Abre a pasta onde o usuário escolheu salvar
            open_file_in_system(output_zip)

            return {"success": True, "message": "Pacote do PS2 salvo com sucesso no diretório escolhido!"}

        except Exception as e:
            return {"success": False, "message": f"Erro interno: {str(e)}"}


# ══════════════════════════════════════════════════════════════════════════════
#  SERVIDOR HTTP LOCAL (COM TRATAMENTO MIME PARA MÓDULOS JAVASCRIPT)
# ══════════════════════════════════════════════════════════════════════════════

class RequestHandler(SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        pass  # Oculta mensagens inúteis no terminal

    def guess_type(self, path):
        if path.endswith(".js"):
            return "application/javascript"
        if path.endswith(".css"):
            return "text/css"
        return super().guess_type(path)


def start_local_server(directory: str, port: int = 58241):
    def run_server():
        os.chdir(directory)
        server = ThreadingHTTPServer(('127.0.0.1', port), RequestHandler)
        server.serve_forever()
    
    t = threading.Thread(target=run_server, daemon=True)
    t.start()
    return f"http://127.0.0.1:{port}/index.html"


# ══════════════════════════════════════════════════════════════════════════════
#  INICIALIZAÇÃO DO PYWEBVIEW E LÓGICA DO PYINSTALLER
# ══════════════════════════════════════════════════════════════════════════════

def main():
    # Detecta se está rodando a partir do executável compilado (.exe) ou script (.py)
    if getattr(sys, 'frozen', False):
        base_dir = os.path.dirname(sys.executable)
    else:
        base_dir = os.path.dirname(os.path.abspath(__file__))

    index_path = os.path.join(base_dir, "index.html")

    if not os.path.exists(index_path):
        print(f"[ERRO] O arquivo index.html não foi encontrado na pasta: {base_dir}")
        print("Certifique-se de colar os arquivos web na mesma pasta do executável.")
        sys.exit(1)

    # Inicia o servidor HTTP em background. Habilita o LocalStorage no navegador.
    port = 58241
    local_url = start_local_server(base_dir, port=port)

    # Diretório para o WebView2 persistir o LocalStorage (Garante que os temas salvos não sumam)
    storage_dir = os.path.join(os.path.expanduser("~"), ".ps2_launcher_studio_data")
    os.makedirs(storage_dir, exist_ok=True)

    # Exposição das funções Python para o JS
    api = StudioAPI()

    window = webview.create_window(
        title="PS2 Launcher Studio — Tema & Conversor TIM2 Oficial",
        url=local_url,
        js_api=api,
        width=1024,
        height=720,
        min_size=(960, 640),
        background_color="#09090b",
        maximized=True
    )

    # private_mode=False permite a escrita persistente do LocalStorage
    webview.start(debug=False, private_mode=False, storage_path=storage_dir)


if __name__ == '__main__':
    main()