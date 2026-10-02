#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import xml.etree.ElementTree as ET
import sys
import os
import subprocess
import shutil
from pathlib import Path

# ============================================================
#  DICCIONARIO DE TRADUCCIONES (16 IDIOMAS)
# ============================================================
TRADUCCIONES = {

    # ============================================================
    # MENÚ ARCHIVO
    # ============================================================
    "Archivo": {"en": "File", "fr": "Fichier", "de": "Datei", "it": "File", "pt": "Arquivo", "zh": "文件", "ar": "ملف", "ru": "Файл", "ja": "ファイル", "ko": "파일", "tr": "Dosya", "pl": "Plik", "nl": "Bestand", "sv": "Arkiv", "hi": "फ़ाइल"},
    "Nuevo lienzo": {"en": "New canvas", "fr": "Nouveau canevas", "de": "Neue Leinwand", "it": "Nuova tela", "pt": "Nova tela", "zh": "新建画布", "ar": "لوحة جديدة", "ru": "Новый холст", "ja": "新しいキャンバス", "ko": "새 캔버스", "tr": "Yeni tuval", "pl": "Nowy płótno", "nl": "Nieuw canvas", "sv": "Ny duk", "hi": "नया कैनवास"},
    "Nuevo lienzo...": {"en": "New canvas...", "fr": "Nouveau canevas...", "de": "Neue Leinwand...", "it": "Nuova tela...", "pt": "Nova tela...", "zh": "新建画布...", "ar": "لوحة جديدة...", "ru": "Новый холст...", "ja": "新しいキャンバス...", "ko": "새 캔버스...", "tr": "Yeni tuval...", "pl": "Nowe płótno...", "nl": "Nieuw canvas...", "sv": "Ny duk...", "hi": "नया कैनवास..."},
    "Abrir...": {"en": "Open...", "fr": "Ouvrir...", "de": "Öffnen...", "it": "Apri...", "pt": "Abrir...", "zh": "打开...", "ar": "فتح...", "ru": "Открыть...", "ja": "開く...", "ko": "열기...", "tr": "Aç...", "pl": "Otwórz...", "nl": "Openen...", "sv": "Öppna...", "hi": "खोलें..."},
    "Guardar": {"en": "Save", "fr": "Enregistrer", "de": "Speichern", "it": "Salva", "pt": "Salvar", "zh": "保存", "ar": "حفظ", "ru": "Сохранить", "ja": "保存", "ko": "저장", "tr": "Kaydet", "pl": "Zapisz", "nl": "Opslaan", "sv": "Spara", "hi": "सहेजें"},
    "Guardar como...": {"en": "Save as...", "fr": "Enregistrer sous...", "de": "Speichern unter...", "it": "Salva come...", "pt": "Salvar como...", "zh": "另存为...", "ar": "حفظ باسم...", "ru": "Сохранить как...", "ja": "名前を付けて保存...", "ko": "다른 이름으로 저장...", "tr": "Farklı kaydet...", "pl": "Zapisz jako...", "nl": "Opslaan als...", "sv": "Spara som...", "hi": "इस रूप में सहेजें..."},
    "Guardar proyecto": {"en": "Save project", "fr": "Enregistrer le projet", "de": "Projekt speichern", "it": "Salva progetto", "pt": "Salvar projeto", "zh": "保存项目", "ar": "حفظ المشروع", "ru": "Сохранить проект", "ja": "プロジェクトを保存", "ko": "프로젝트 저장", "tr": "Projeyi kaydet", "pl": "Zapisz projekt", "nl": "Project opslaan", "sv": "Spara projekt", "hi": "प्रोजेक्ट सहेजें"},
    "Guardar proyecto como...": {"en": "Save project as...", "fr": "Enregistrer le projet sous...", "de": "Projekt speichern unter...", "it": "Salva progetto come...", "pt": "Salvar projeto como...", "zh": "项目另存为...", "ar": "حفظ المشروع باسم...", "ru": "Сохранить проект как...", "ja": "プロジェクトを名前を付けて保存...", "ko": "프로젝트를 다른 이름으로 저장...", "tr": "Proyecto farklı kaydet...", "pl": "Zapisz projekt jako...", "nl": "Project opslaan als...", "sv": "Spara projekt som...", "hi": "प्रोजेक्ट को इस रूप में सहेजें..."},
    "Insertar imagen (objeto seleccionable)": {"en": "Insert image (selectable object)", "fr": "Insérer une image (objet sélectionnable)", "de": "Bild einfügen (auswählbares Objekt)", "it": "Inserisci immagine (oggetto selezionabile)", "pt": "Inserir imagem (objeto selecionável)", "zh": "插入图像（可选择对象）", "ar": "إدراج صورة (كائن قابل للتحديد)", "ru": "Вставить изображение (выделяемый объект)", "ja": "画像を挿入（選択可能なオブジェクト）", "ko": "이미지 삽입 (선택 가능한 개체)", "tr": "Resim ekle (seçilebilir nesne)", "pl": "Wstaw obraz (obiekt wybieralny)", "nl": "Afbeelding invoegen (selecteerbaar object)", "sv": "Infoga bild (valbart objekt)", "hi": "छवि सम्मिलित करें (चयन योग्य वस्तु)"},
    "Insertar imagen como capa nueva": {"en": "Insert image as new layer", "fr": "Insérer une image comme nouveau calque", "de": "Bild als neue Ebene einfügen", "it": "Inserisci immagine come nuovo livello", "pt": "Inserir imagem como nova camada", "zh": "以新图层插入图像", "ar": "إدراج صورة كطبقة جديدة", "ru": "Вставить изображение как новый слой", "ja": "画像を新規レイヤーとして挿入", "ko": "이미지를 새 레이어로 삽입", "tr": "Resmi yeni katman olarak ekle", "pl": "Wstaw obraz jako nową warstwę", "nl": "Afbeelding invoegen als nieuwe laag", "sv": "Infoga bild som nytt lager", "hi": "छवि को नई परत के रूप में सम्मिलित करें"},
    "Abrir imagen como fondo": {"en": "Open image as background", "fr": "Ouvrir une image comme fond", "de": "Bild als Hintergrund öffnen", "it": "Apri immagine come sfondo", "pt": "Abrir imagem como fundo", "zh": "以背景打开图像", "ar": "فتح صورة كخلفية", "ru": "Открыть изображение как фон", "ja": "画像を背景として開く", "ko": "이미지를 배경으로 열기", "tr": "Resmi arka plan olarak aç", "pl": "Otwórz obraz jako tło", "nl": "Afbeelding als achtergrond openen", "sv": "Öppna bild som bakgrund", "hi": "छवि को पृष्ठभूमि के रूप में खोलें"},
    "Salir de la aplicacion": {"en": "Exit application", "fr": "Quitter l'application", "de": "Anwendung beenden", "it": "Esci dall'applicazione", "pt": "Sair da aplicação", "zh": "退出应用程序", "ar": "الخروج من التطبيق", "ru": "Выйти из приложения", "ja": "アプリケーションを終了", "ko": "응용 프로그램 종료", "tr": "Uygulamadan çık", "pl": "Zamknij aplikację", "nl": "Applicatie afsluiten", "sv": "Avsluta applikationen", "hi": "एप्लिकेशन से बाहर निकलें"},

    # ============================================================
    # MENÚ VER
    # ============================================================
    "Ver": {"en": "View", "fr": "Voir", "de": "Ansicht", "it": "Visualizza", "pt": "Ver", "zh": "查看", "ar": "عرض", "ru": "Вид", "ja": "表示", "ko": "보기", "tr": "Görünüm", "pl": "Widok", "nl": "Beeld", "sv": "Visa", "hi": "देखें"},
    "Pantalla Completa": {"en": "Full Screen", "fr": "Plein écran", "de": "Vollbild", "it": "Schermo intero", "pt": "Tela cheia", "zh": "全屏", "ar": "ملء الشاشة", "ru": "Полный экран", "ja": "全画面", "ko": "전체 화면", "tr": "Tam ekran", "pl": "Pełny ekran", "nl": "Volledig scherm", "sv": "Helskärm", "hi": "पूर्ण स्क्रीन"},
    "Maximizar Ventana": {"en": "Maximize Window", "fr": "Maximiser la fenêtre", "de": "Fenster maximieren", "it": "Massimizza finestra", "pt": "Maximizar janela", "zh": "最大化窗口", "ar": "تكبير النافذة", "ru": "Развернуть окно", "ja": "ウィンドウを最大化", "ko": "창 최대화", "tr": "Pencereyi büyüt", "pl": "Maksymalizuj okno", "nl": "Venster maximaliseren", "sv": "Maximera fönster", "hi": "विंडो अधिकतम करें"},
    "Cuadricula": {"en": "Grid", "fr": "Grille", "de": "Raster", "it": "Griglia", "pt": "Grade", "zh": "网格", "ar": "شبكة", "ru": "Сетка", "ja": "グリッド", "ko": "그리드", "tr": "Izgara", "pl": "Siatka", "nl": "Raster", "sv": "Rutnät", "hi": "ग्रिड"},
    "Apagado": {"en": "Off", "fr": "Désactivé", "de": "Aus", "it": "Spento", "pt": "Desligado", "zh": "关闭", "ar": "إيقاف", "ru": "Выкл", "ja": "オフ", "ko": "끄기", "tr": "Kapalı", "pl": "Wyłączone", "nl": "Uit", "sv": "Av", "hi": "बंद"},

    # ============================================================
    # CONFIGURACIONES
    # ============================================================
    "Modo Normal": {"en": "Normal Mode", "fr": "Mode normal", "de": "Normaler Modus", "it": "Modalità normale", "pt": "Modo normal", "zh": "正常模式", "ar": "الوضع العادي", "ru": "Обычный режим", "ja": "通常モード", "ko": "일반 모드", "tr": "Normal Mod", "pl": "Tryb normalny", "nl": "Normale modus", "sv": "Normalläge", "hi": "सामान्य मोड"},
    "Modo Avanzado": {"en": "Advanced Mode", "fr": "Mode avancé", "de": "Erweiterter Modus", "it": "Modalità avanzata", "pt": "Modo avançado", "zh": "高级模式", "ar": "الوضع المتقدم", "ru": "Расширенный режим", "ja": "詳細モード", "ko": "고급 모드", "tr": "Gelişmiş Mod", "pl": "Tryb zaawansowany", "nl": "Geavanceerde modus", "sv": "Avancerat läge", "hi": "उन्नत मोड"},
    "Modo Pixel Art": {"en": "Pixel Art Mode", "fr": "Mode Pixel Art", "de": "Pixel Art Modus", "it": "Modalità Pixel Art", "pt": "Modo Pixel Art", "zh": "像素艺术模式", "ar": "وضع بكسل آرت", "ru": "Режим Pixel Art", "ja": "ピクセルアートモード", "ko": "픽셀 아트 모드", "tr": "Pixel Art Modu", "pl": "Tryb Pixel Art", "nl": "Pixel Art modus", "sv": "Pixel Art-läge", "hi": "पिक्सेल आर्ट मोड"},
    "Sensibilidad del Ratón": {"en": "Mouse Sensitivity", "fr": "Sensibilité de la souris", "de": "Mausempfindlichkeit", "it": "Sensibilità del mouse", "pt": "Sensibilidade do mouse", "zh": "鼠标灵敏度", "ar": "حساسية الماوس", "ru": "Чувствительность мыши", "ja": "マウスの感度", "ko": "마우스 감도", "tr": "Fare hassasiyeti", "pl": "Czułość myszy", "nl": "Muisgevoeligheid", "sv": "Muskänslighet", "hi": "माउस संवेदनशीलता"},
    "Sensibilidad del Raton": {"en": "Mouse Sensitivity", "fr": "Sensibilité de la souris", "de": "Mausempfindlichkeit", "it": "Sensibilità del mouse", "pt": "Sensibilidade do mouse", "zh": "鼠标灵敏度", "ar": "حساسية الماوس", "ru": "Чувствительность мыши", "ja": "マウス感度", "ko": "마우스 감도", "tr": "Fare hassasiyeti", "pl": "Czułość myszy", "nl": "Muisgevoeligheid", "sv": "Muskänslighet", "hi": "माउस संवेदनशीलता"},
    "Baja": {"en": "Low", "fr": "Basse", "de": "Niedrig", "it": "Bassa", "pt": "Baixa", "zh": "低", "ar": "منخفض", "ru": "Низкая", "ja": "低", "ko": "낮음", "tr": "Düşük", "pl": "Niska", "nl": "Laag", "sv": "Låg", "hi": "कम"},
    "Normal": {"en": "Normal", "fr": "Normale", "de": "Normal", "it": "Normale", "pt": "Normal", "zh": "正常", "ar": "عادي", "ru": "Нормальная", "ja": "通常", "ko": "보통", "tr": "Normal", "pl": "Normalna", "nl": "Normaal", "sv": "Normal", "hi": "सामान्य"},
    "Alta": {"en": "High", "fr": "Haute", "de": "Hoch", "it": "Alta", "pt": "Alta", "zh": "高", "ar": "عالي", "ru": "Высокая", "ja": "高", "ko": "높음", "tr": "Yüksek", "pl": "Wysoka", "nl": "Hoog", "sv": "Hög", "hi": "उच्च"},
    "Transparencia del Fondo": {"en": "Background Transparency", "fr": "Transparence du fond", "de": "Hintergrundtransparenz", "it": "Trasparenza dello sfondo", "pt": "Transparência do fundo", "zh": "背景透明度", "ar": "شفافية الخلفية", "ru": "Прозрачность фона", "ja": "背景の透明度", "ko": "배경 투명도", "tr": "Arka plan şeffaflığı", "pl": "Przezroczystość tła", "nl": "Achtergrondtransparantie", "sv": "Bakgrundstransparens", "hi": "पृष्ठभूमि पारदर्शिता"},
    "Opaco (100%)": {"en": "Opaque (100%)", "fr": "Opaque (100%)", "de": "Undurchsichtig (100%)", "it": "Opaco (100%)", "pt": "Opaco (100%)", "zh": "不透明（100%）", "ar": "معتم (100%)", "ru": "Непрозрачный (100%)", "ja": "不透明 (100%)", "ko": "불투명 (100%)", "tr": "Opak (100%)", "pl": "Nieprzezroczysty (100%)", "nl": "Ondoorzichtig (100%)", "sv": "Ogenomskinlig (100%)", "hi": "अपारदर्शी (100%)"},
    "85% (Muy Ligero)": {"en": "85% (Very Light)", "fr": "85 % (Très léger)", "de": "85 % (Sehr leicht)", "it": "85% (Molto leggero)", "pt": "85% (Muito leve)", "zh": "85%（非常轻）", "ar": "85% (خفيف جدًا)", "ru": "85% (Очень лёгкая)", "ja": "85%（非常に薄い）", "ko": "85% (매우 옅음)", "tr": "%85 (Çok hafif)", "pl": "85% (Bardzo lekka)", "nl": "85% (Zeer licht)", "sv": "85% (Mycket lätt)", "hi": "85% (बहुत हल्का)"},
    "75% (Ligero)": {"en": "75% (Light)", "fr": "75 % (Léger)", "de": "75 % (Leicht)", "it": "75% (Leggero)", "pt": "75% (Leve)", "zh": "75%（轻）", "ar": "75% (خفيف)", "ru": "75% (Лёгкая)", "ja": "75%（薄い）", "ko": "75% (옅음)", "tr": "%75 (Hafif)", "pl": "75% (Lekka)", "nl": "75% (Licht)", "sv": "75% (Lätt)", "hi": "75% (हल्का)"},
    "50% (Medio)": {"en": "50% (Medium)", "fr": "50 % (Moyen)", "de": "50 % (Mittel)", "it": "50% (Medio)", "pt": "50% (Médio)", "zh": "50%（中等）", "ar": "50% (متوسط)", "ru": "50% (Средняя)", "ja": "50%（中間）", "ko": "50% (보통)", "tr": "%50 (Orta)", "pl": "50% (Średnia)", "nl": "50% (Gemiddeld)", "sv": "50% (Medel)", "hi": "50% (मध्यम)"},
    "25% (Fuerte)": {"en": "25% (Strong)", "fr": "25 % (Fort)", "de": "25 % (Stark)", "it": "25% (Forte)", "pt": "25% (Forte)", "zh": "25%（强）", "ar": "25% (قوي)", "ru": "25% (Сильная)", "ja": "25%（濃い）", "ko": "25% (진함)", "tr": "%25 (Güçlü)", "pl": "25% (Silna)", "nl": "25% (Sterk)", "sv": "25% (Stark)", "hi": "25% (मज़बूत)"},
    "10% (Muy Fuerte)": {"en": "10% (Very Strong)", "fr": "10 % (Très fort)", "de": "10 % (Sehr stark)", "it": "10% (Molto forte)", "pt": "10% (Muito forte)", "zh": "10%（非常强）", "ar": "10% (قوي جدًا)", "ru": "10% (Очень сильная)", "ja": "10%（非常に濃い）", "ko": "10% (매우 진함)", "tr": "%10 (Çok güçlü)", "pl": "10% (Bardzo silna)", "nl": "10% (Zeer sterk)", "sv": "10% (Mycket stark)", "hi": "10% (बहुत मज़बूत)"},
    "0% (Ver Wallpaper)": {"en": "0% (Show Wallpaper)", "fr": "0 % (Voir le fond d'écran)", "de": "0 % (Hintergrund anzeigen)", "it": "0% (Mostra sfondo)", "pt": "0% (Ver papel de parede)", "zh": "0%（显示壁纸）", "ar": "0% (عرض الخلفية)", "ru": "0% (Показать обои)", "ja": "0%（壁紙を表示）", "ko": "0% (배경화면 보기)", "tr": "%0 (Duvar kağıdını göster)", "pl": "0% (Pokaż tapetę)", "nl": "0% (Achtergrond tonen)", "sv": "0% (Visa bakgrund)", "hi": "0% (वॉलपेपर दिखाएँ)"},
    "Personalizado...": {"en": "Custom...", "fr": "Personnalisé...", "de": "Benutzerdefiniert...", "it": "Personalizzato...", "pt": "Personalizado...", "zh": "自定义...", "ar": "مخصص...", "ru": "Настраиваемый...", "ja": "カスタム...", "ko": "사용자 정의...", "tr": "Özel...", "pl": "Niestandardowy...", "nl": "Aangepast...", "sv": "Anpassad...", "hi": "कस्टम..."},
    "Idiomas / Languages": {"en": "Languages", "fr": "Langues", "de": "Sprachen", "it": "Lingue", "pt": "Idiomas", "zh": "语言", "ar": "اللغات", "ru": "Языки", "ja": "言語", "ko": "언어", "tr": "Diller", "pl": "Języki", "nl": "Talen", "sv": "Språk", "hi": "भाषाएं"},
    "Ayuda / Informacion": {"en": "Help / Information", "fr": "Aide / Informations", "de": "Hilfe / Informationen", "it": "Aiuto / Informazioni", "pt": "Ajuda / Informações", "zh": "帮助 / 信息", "ar": "مساعدة / معلومات", "ru": "Помощь / Информация", "ja": "ヘルプ / 情報", "ko": "도움말 / 정보", "tr": "Yardım / Bilgi", "pl": "Pomoc / Informacje", "nl": "Help / Informatie", "sv": "Hjälp / Information", "hi": "सहायता / जानकारी"},
    "Ayuda": {"en": "Help", "fr": "Aide", "de": "Hilfe", "it": "Aiuto", "pt": "Ajuda", "zh": "帮助", "ar": "مساعدة", "ru": "Помощь", "ja": "ヘルプ", "ko": "도움말", "tr": "Yardım", "pl": "Pomoc", "nl": "Help", "sv": "Hjälp", "hi": "सहायता"},
    "Acerca de Paintlux Studio": {"en": "About Paintlux Studio", "fr": "À propos de Paintlux Studio", "de": "Über Paintlux Studio", "it": "Informazioni su Paintlux Studio", "pt": "Sobre o Paintlux Studio", "zh": "关于 Paintlux Studio", "ar": "حول Paintlux Studio", "ru": "О программе Paintlux Studio", "ja": "Paintlux Studio について", "ko": "Paintlux Studio 정보", "tr": "Paintlux Studio Hakkında", "pl": "O Paintlux Studio", "nl": "Over Paintlux Studio", "sv": "Om Paintlux Studio", "hi": "Paintlux Studio के बारे में"},

    # ============================================================
    # RIBBON - Portapapeles
    # ============================================================
    "Portapapeles": {"en": "Clipboard", "fr": "Presse-papiers", "de": "Zwischenablage", "it": "Appunti", "pt": "Área de transferência", "zh": "剪贴板", "ar": "الحافظة", "ru": "Буфер обмена", "ja": "クリップボード", "ko": "클립보드", "tr": "Pano", "pl": "Schowek", "nl": "Klembord", "sv": "Urklipp", "hi": "क्लिपबोर्ड"},
    "Pegar": {"en": "Paste", "fr": "Coller", "de": "Einfügen", "it": "Incolla", "pt": "Colar", "zh": "粘贴", "ar": "لصق", "ru": "Вставить", "ja": "貼り付け", "ko": "붙여넣기", "tr": "Yapıştır", "pl": "Wklej", "nl": "Plakken", "sv": "Klistra in", "hi": "चिपकाएं"},
    " Copiar": {"en": " Copy", "fr": " Copier", "de": " Kopieren", "it": " Copia", "pt": " Copiar", "zh": " 复制", "ar": " نسخ", "ru": " Копировать", "ja": " コピー", "ko": " 복사", "tr": " Kopyala", "pl": " Kopiuj", "nl": " Kopiëren", "sv": " Kopiera", "hi": " कॉपी करें"},
    " Cortar": {"en": " Cut", "fr": " Couper", "de": " Ausschneiden", "it": " Taglia", "pt": " Cortar", "zh": " 剪切", "ar": " قص", "ru": " Вырезать", "ja": " 切り取り", "ko": " 잘라내기", "tr": " Kes", "pl": " Wytnij", "nl": " Knippen", "sv": " Klipp ut", "hi": " काटें"},

    # ============================================================
    # RIBBON - Imagen y Herramientas
    # ============================================================
    "Imagen": {"en": "Image", "fr": "Image", "de": "Bild", "it": "Immagine", "pt": "Imagem", "zh": "图像", "ar": "صورة", "ru": "Изображение", "ja": "画像", "ko": "이미지", "tr": "Resim", "pl": "Obraz", "nl": "Afbeelding", "sv": "Bild", "hi": "छवि"},
    "Seleccionar": {"en": "Select", "fr": "Sélectionner", "de": "Auswählen", "it": "Seleziona", "pt": "Selecionar", "zh": "选择", "ar": "تحديد", "ru": "Выделить", "ja": "選択", "ko": "선택", "tr": "Seç", "pl": "Zaznacz", "nl": "Selecteren", "sv": "Välj", "hi": "चुनें"},
    "Seleccion Libre": {"en": "Free Select", "fr": "Sélection libre", "de": "Freie Auswahl", "it": "Selezione libera", "pt": "Seleção livre", "zh": "自由选择", "ar": "تحديد حر", "ru": "Свободное выделение", "ja": "自由選択", "ko": "자유 선택", "tr": "Serbest seçim", "pl": "Swobodne zaznaczanie", "nl": "Vrije selectie", "sv": "Fri markering", "hi": "मुक्त चयन"},
    "Herramientas y Pinceles": {"en": "Tools and Brushes", "fr": "Outils et pinceaux", "de": "Werkzeuge und Pinsel", "it": "Strumenti e pennelli", "pt": "Ferramentas e pincéis", "zh": "工具和画笔", "ar": "الأدوات والفرش", "ru": "Инструменты и кисти", "ja": "ツールとブラシ", "ko": "도구 및 브러시", "tr": "Araçlar ve Fırçalar", "pl": "Narzędzia i pędzle", "nl": "Gereedschappen en penselen", "sv": "Verktyg och penslar", "hi": "उपकरण और ब्रश"},
    "Formas": {"en": "Shapes", "fr": "Formes", "de": "Formen", "it": "Forme", "pt": "Formas", "zh": "形状", "ar": "أشكال", "ru": "Фигуры", "ja": "図形", "ko": "도형", "tr": "Şekiller", "pl": "Kształty", "nl": "Vormen", "sv": "Former", "hi": "आकृतियाँ"},
    "Propiedades": {"en": "Properties", "fr": "Propriétés", "de": "Eigenschaften", "it": "Proprietà", "pt": "Propriedades", "zh": "属性", "ar": "خصائص", "ru": "Свойства", "ja": "プロパティ", "ko": "속성", "tr": "Özellikler", "pl": "Właściwości", "nl": "Eigenschappen", "sv": "Egenskaper", "hi": "गुण"},
    "Opacidad:": {"en": "Opacity:", "fr": "Opacité :", "de": "Deckkraft:", "it": "Opacità:", "pt": "Opacidade:", "zh": "不透明度：", "ar": "العتامة:", "ru": "Непрозрачность:", "ja": "不透明度：", "ko": "불투명도:", "tr": "Opaklık:", "pl": "Nieprzezroczystość:", "nl": "Dekking:", "sv": "Opacitet:", "hi": "अपारदर्शिता:"},
    "Tamaño:": {"en": "Size:", "fr": "Taille :", "de": "Größe:", "it": "Dimensione:", "pt": "Tamanho:", "zh": "大小：", "ar": "الحجم:", "ru": "Размер:", "ja": "サイズ：", "ko": "크기:", "tr": "Boyut:", "pl": "Rozmiar:", "nl": "Grootte:", "sv": "Storlek:", "hi": "आकार:"},
    "Tamano:": {"en": "Size:", "fr": "Taille :", "de": "Größe:", "it": "Dimensione:", "pt": "Tamanho:", "zh": "大小：", "ar": "الحجم:", "ru": "Размер:", "ja": "サイズ：", "ko": "크기:", "tr": "Boyut:", "pl": "Rozmiar:", "nl": "Grootte:", "sv": "Storlek:", "hi": "आकार:"},

    # ============================================================
    # RIBBON - Animación
    # ============================================================
    "Animacion": {"en": "Animation", "fr": "Animation", "de": "Animation", "it": "Animazione", "pt": "Animação", "zh": "动画", "ar": "رسوم متحركة", "ru": "Анимация", "ja": "アニメーション", "ko": "애니메이션", "tr": "Animasyon", "pl": "Animacja", "nl": "Animatie", "sv": "Animation", "hi": "एनीमेशन"},
    "Vel:": {"en": "Speed:", "fr": "Vitesse :", "de": "Geschwindigkeit:", "it": "Velocità:", "pt": "Velocidade:", "zh": "速度：", "ar": "السرعة:", "ru": "Скорость:", "ja": "速度：", "ko": "속도:", "tr": "Hız:", "pl": "Prędkość:", "nl": "Snelheid:", "sv": "Hastighet:", "hi": "गति:"},
    "+ Frame": {"en": "+ Frame", "fr": "+ Image", "de": "+ Frame", "it": "+ Frame", "pt": "+ Quadro", "zh": "+ 帧", "ar": "+ إطار", "ru": "+ Кадр", "ja": "+ フレーム", "ko": "+ 프레임", "tr": "+ Kare", "pl": "+ Klatka", "nl": "+ Frame", "sv": "+ Bildruta", "hi": "+ फ़्रेम"},
    "Duplicar": {"en": "Duplicate", "fr": "Dupliquer", "de": "Duplizieren", "it": "Duplica", "pt": "Duplicar", "zh": "复制", "ar": "تكرار", "ru": "Дублировать", "ja": "複製", "ko": "복제", "tr": "Çoğalt", "pl": "Duplikuj", "nl": "Dupliceren", "sv": "Duplicera", "hi": "डुप्लिकेट"},
    "Eliminar": {"en": "Delete", "fr": "Supprimer", "de": "Löschen", "it": "Elimina", "pt": "Excluir", "zh": "删除", "ar": "حذف", "ru": "Удалить", "ja": "削除", "ko": "삭제", "tr": "Sil", "pl": "Usuń", "nl": "Verwijderen", "sv": "Ta bort", "hi": "हटाएँ"},
    "Play": {"en": "Play", "fr": "Lecture", "de": "Abspielen", "it": "Riproduci", "pt": "Reproduzir", "zh": "播放", "ar": "تشغيل", "ru": "Воспроизвести", "ja": "再生", "ko": "재생", "tr": "Oynat", "pl": "Odtwórz", "nl": "Afspelen", "sv": "Spela", "hi": "चलाएँ"},
    "Stop": {"en": "Stop", "fr": "Arrêter", "de": "Stopp", "it": "Ferma", "pt": "Parar", "zh": "停止", "ar": "إيقاف", "ru": "Стоп", "ja": "停止", "ko": "중지", "tr": "Durdur", "pl": "Stop", "nl": "Stoppen", "sv": "Stoppa", "hi": "रोकें"},

    # ============================================================
    # RIBBON - Colores
    # ============================================================
    "Colores": {"en": "Colors", "fr": "Couleurs", "de": "Farben", "it": "Colori", "pt": "Cores", "zh": "颜色", "ar": "ألوان", "ru": "Цвета", "ja": "色", "ko": "색상", "tr": "Renkler", "pl": "Kolory", "nl": "Kleuren", "sv": "Färger", "hi": "रंग"},
    "Editar Colores": {"en": "Edit Colors", "fr": "Modifier les couleurs", "de": "Farben bearbeiten", "it": "Modifica colori", "pt": "Editar cores", "zh": "编辑颜色", "ar": "تحرير الألوان", "ru": "Изменить цвета", "ja": "色を編集", "ko": "색상 편집", "tr": "Renkleri düzenle", "pl": "Edytuj kolory", "nl": "Kleuren bewerken", "sv": "Redigera färger", "hi": "रंग संपादित करें"},

    # ============================================================
    # SIDEBAR IZQUIERDA - Herramientas avanzadas
    # ============================================================
    "Seleccion y Retoque": {"en": "Selection and Retouch", "fr": "Sélection et retouche", "de": "Auswahl und Retusche", "it": "Selezione e ritocco", "pt": "Seleção e retoque", "zh": "选择和修饰", "ar": "التحديد والرتوش", "ru": "Выделение и ретушь", "ja": "選択とレタッチ", "ko": "선택 및 리터치", "tr": "Seçim ve Rötuş", "pl": "Zaznaczanie i retusz", "nl": "Selectie en retouch", "sv": "Markering och retusch", "hi": "चयन और रिटच"},
    "Vectores": {"en": "Vectors", "fr": "Vecteurs", "de": "Vektoren", "it": "Vettori", "pt": "Vetores", "zh": "矢量", "ar": "متجهات", "ru": "Векторы", "ja": "ベクター", "ko": "벡터", "tr": "Vektörler", "pl": "Wektory", "nl": "Vectoren", "sv": "Vektorer", "hi": "वेक्टर"},
    "Pluma Bezier": {"en": "Bezier Pen", "fr": "Plume Bézier", "de": "Bezier-Feder", "it": "Penna Bézier", "pt": "Caneta Bézier", "zh": "贝塞尔钢笔", "ar": "قلم بيزييه", "ru": "Перо Безье", "ja": "ベジェペン", "ko": "베지어 펜", "tr": "Bezier Kalem", "pl": "Pióro Bézier", "nl": "Bezier-pen", "sv": "Bezier-penna", "hi": "बेज़ियर पेन"},

    # ============================================================
    # SIDEBAR - Capas
    # ============================================================
    "Capas": {"en": "Layers", "fr": "Calques", "de": "Ebenen", "it": "Livelli", "pt": "Camadas", "zh": "图层", "ar": "طبقات", "ru": "Слои", "ja": "レイヤー", "ko": "레이어", "tr": "Katmanlar", "pl": "Warstwy", "nl": "Lagen", "sv": "Lager", "hi": "परतें"},
    "Fusion:": {"en": "Blend:", "fr": "Fusion :", "de": "Überblendung:", "it": "Fusione:", "pt": "Fusão:", "zh": "混合：", "ar": "المزج:", "ru": "Наложение:", "ja": "合成モード：", "ko": "블렌딩:", "tr": "Karışım:", "pl": "Mieszanie:", "nl": "Menging:", "sv": "Blandning:", "hi": "मिश्रण:"},
    "Disolver": {"en": "Dissolve", "fr": "Dissoudre", "de": "Auflösen", "it": "Dissolvi", "pt": "Dissolver", "zh": "溶解", "ar": "إذابة", "ru": "Растворение", "ja": "ディザ合成", "ko": "디졸브", "tr": "Çöz", "pl": "Rozpuszczenie", "nl": "Dissolve", "sv": "Upplös", "hi": "विघटन"},
    "Oscurecer": {"en": "Darken", "fr": "Obscurcir", "de": "Abdunkeln", "it": "Scurisci", "pt": "Escurecer", "zh": "变暗", "ar": "تعتيم", "ru": "Замена тёмным", "ja": "暗く", "ko": "어둡게", "tr": "Karart", "pl": "Przyciemnij", "nl": "Donkerder", "sv": "Mörkare", "hi": "गहरा"},
    "Multiplicar": {"en": "Multiply", "fr": "Multiplier", "de": "Multiplizieren", "it": "Moltiplica", "pt": "Multiplicar", "zh": "正片叠底", "ar": "ضرب", "ru": "Умножение", "ja": "乗算", "ko": "곱하기", "tr": "Çarp", "pl": "Mnożenie", "nl": "Vermenigvuldigen", "sv": "Multiplicera", "hi": "गुणा"},
    "Aclarar": {"en": "Lighten", "fr": "Éclaircir", "de": "Aufhellen", "it": "Schiarisci", "pt": "Clarear", "zh": "变亮", "ar": "تفتيح", "ru": "Замена светлым", "ja": "明るく", "ko": "밝게", "tr": "Aydınlat", "pl": "Rozjaśnij", "nl": "Lichter", "sv": "Ljusare", "hi": "हल्का"},
    "Trama": {"en": "Screen", "fr": "Trame", "de": "Negativ multiplizieren", "it": "Schermo", "pt": "Tela", "zh": "滤色", "ar": "شاشة", "ru": "Экран", "ja": "スクリーン", "ko": "스크린", "tr": "Ekran", "pl": "Ekran", "nl": "Screen", "sv": "Screen", "hi": "स्क्रीन"},
    "Sobreexponer color": {"en": "Color Dodge", "fr": "Densité couleur -", "de": "Farbig abwedeln", "it": "Scherma colore", "pt": "Subexposição de cor", "zh": "颜色减淡", "ar": "تفتيح اللون", "ru": "Осветление основы", "ja": "覆い焼きカラー", "ko": "컬러 닷지", "tr": "Renk soldurma", "pl": "Rozjaśnianie koloru", "nl": "Kleur tegenhouden", "sv": "Färgdodge", "hi": "रंग डॉज"},
    "Superponer": {"en": "Overlay", "fr": "Superposition", "de": "Ineinanderkopieren", "it": "Sovrapposto", "pt": "Sobrepor", "zh": "叠加", "ar": "تراكب", "ru": "Перекрытие", "ja": "オーバーレイ", "ko": "오버레이", "tr": "Kaplama", "pl": "Nakładka", "nl": "Overlay", "sv": "Överlägg", "hi": "ओवरले"},
    "Luz suave": {"en": "Soft Light", "fr": "Lumière douce", "de": "Weiches Licht", "it": "Luce soffusa", "pt": "Luz suave", "zh": "柔光", "ar": "ضوء ناعم", "ru": "Мягкий свет", "ja": "ソフトライト", "ko": "소프트 라이트", "tr": "Yumuşak ışık", "pl": "Światło miękkie", "nl": "Zacht licht", "sv": "Mjukt ljus", "hi": "नरम प्रकाश"},
    "Luz fuerte": {"en": "Hard Light", "fr": "Lumière dure", "de": "Hartes Licht", "it": "Luce intensa", "pt": "Luz intensa", "zh": "强光", "ar": "ضوء قوي", "ru": "Жёсткий свет", "ja": "ハードライト", "ko": "하드 라이트", "tr": "Sert ışık", "pl": "Ostre światło", "nl": "Hard licht", "sv": "Hårt ljus", "hi": "कठोर प्रकाश"},
    "Diferencia": {"en": "Difference", "fr": "Différence", "de": "Differenz", "it": "Differenza", "pt": "Diferença", "zh": "差值", "ar": "فرق", "ru": "Разница", "ja": "差の絶対値", "ko": "차이", "tr": "Fark", "pl": "Różnica", "nl": "Verschil", "sv": "Skillnad", "hi": "अंतर"},
    "Exclusion": {"en": "Exclusion", "fr": "Exclusion", "de": "Ausschluss", "it": "Esclusione", "pt": "Exclusão", "zh": "排除", "ar": "استبعاد", "ru": "Исключение", "ja": "除外", "ko": "제외", "tr": "Hariç tutma", "pl": "Wykluczenie", "nl": "Uitsluiting", "sv": "Uteslutning", "hi": "बहिष्करण"},
    "Bloquear capa": {"en": "Lock layer", "fr": "Verrouiller le calque", "de": "Ebene sperren", "it": "Blocca livello", "pt": "Bloquear camada", "zh": "锁定图层", "ar": "قفل الطبقة", "ru": "Заблокировать слой", "ja": "レイヤーをロック", "ko": "레이어 잠금", "tr": "Katmanı kilitle", "pl": "Zablokuj warstwę", "nl": "Laag vergrendelen", "sv": "Lås lager", "hi": "परत लॉक करें"},
    "Capa: %1": {"en": "Layer: %1", "fr": "Calque : %1", "de": "Ebene: %1", "it": "Livello: %1", "pt": "Camada: %1", "zh": "图层：%1", "ar": "الطبقة: %1", "ru": "Слой: %1", "ja": "レイヤー：%1", "ko": "레이어: %1", "tr": "Katman: %1", "pl": "Warstwa: %1", "nl": "Laag: %1", "sv": "Lager: %1", "hi": "परत: %1"},
    "Capa %1": {"en": "Layer %1", "fr": "Calque %1", "de": "Ebene %1", "it": "Livello %1", "pt": "Camada %1", "zh": "图层 %1", "ar": "الطبقة %1", "ru": "Слой %1", "ja": "レイヤー %1", "ko": "레이어 %1", "tr": "Katman %1", "pl": "Warstwa %1", "nl": "Laag %1", "sv": "Lager %1", "hi": "परत %1"},
    "Fondo": {"en": "Background", "fr": "Fond", "de": "Hintergrund", "it": "Sfondo", "pt": "Fundo", "zh": "背景", "ar": "الخلفية", "ru": "Фон", "ja": "背景", "ko": "배경", "tr": "Arka plan", "pl": "Tło", "nl": "Achtergrond", "sv": "Bakgrund", "hi": "पृष्ठभूमि"},
    "Capa Importada": {"en": "Imported Layer", "fr": "Calque importé", "de": "Importierte Ebene", "it": "Livello importato", "pt": "Camada importada", "zh": "导入的图层", "ar": "الطبقة المستوردة", "ru": "Импортированный слой", "ja": "インポートされたレイヤー", "ko": "가져온 레이어", "tr": "İçe aktarılan katman", "pl": "Importowana warstwa", "nl": "Geïmporteerde laag", "sv": "Importerat lager", "hi": "आयातित परत"},

    # ============================================================
    # BARRA INFERIOR
    # ============================================================
    "Rotar 90 Derecha": {"en": "Rotate 90° Right", "fr": "Rotation 90° droite", "de": "90° rechts drehen", "it": "Ruota 90° a destra", "pt": "Girar 90° à direita", "zh": "向右旋转 90°", "ar": "تدوير 90° يمينًا", "ru": "Повернуть на 90° вправо", "ja": "右に 90° 回転", "ko": "오른쪽으로 90° 회전", "tr": "90° sağa döndür", "pl": "Obróć 90° w prawo", "nl": "90° rechtsom draaien", "sv": "Rotera 90° höger", "hi": "90° दाएँ घुमाएँ"},
    "Rotar 90 Izquierda": {"en": "Rotate 90° Left", "fr": "Rotation 90° gauche", "de": "90° links drehen", "it": "Ruota 90° a sinistra", "pt": "Girar 90° à esquerda", "zh": "向左旋转 90°", "ar": "تدوير 90° يسارًا", "ru": "Повернуть на 90° влево", "ja": "左に 90° 回転", "ko": "왼쪽으로 90° 회전", "tr": "90° sola döndür", "pl": "Obróć 90° w lewo", "nl": "90° linksom draaien", "sv": "Rotera 90° vänster", "hi": "90° बाएँ घुमाएँ"},
    "Rotar 180": {"en": "Rotate 180°", "fr": "Rotation 180°", "de": "180° drehen", "it": "Ruota 180°", "pt": "Girar 180°", "zh": "旋转 180°", "ar": "تدوير 180°", "ru": "Повернуть на 180°", "ja": "180° 回転", "ko": "180° 회전", "tr": "180° döndür", "pl": "Obróć 180°", "nl": "180° draaien", "sv": "Rotera 180°", "hi": "180° घुमाएँ"},
    "Voltear Horizontal": {"en": "Flip Horizontal", "fr": "Retourner horizontalement", "de": "Horizontal spiegeln", "it": "Rifletti orizzontalmente", "pt": "Inverter horizontalmente", "zh": "水平翻转", "ar": "قلب أفقي", "ru": "Отразить по горизонтали", "ja": "水平方向に反転", "ko": "수평 뒤집기", "tr": "Yatay çevir", "pl": "Odwróć w poziomie", "nl": "Horizontaal spiegelen", "sv": "Vänd horisontellt", "hi": "क्षैतिज पलटें"},
    "Voltear Vertical": {"en": "Flip Vertical", "fr": "Retourner verticalement", "de": "Vertikal spiegeln", "it": "Rifletti verticalmente", "pt": "Inverter verticalmente", "zh": "垂直翻转", "ar": "قلب عمودي", "ru": "Отразить по вертикали", "ja": "垂直方向に反転", "ko": "수직 뒤집기", "tr": "Dikey çevir", "pl": "Odwróć w pionie", "nl": "Verticaal spiegelen", "sv": "Vänd vertikalt", "hi": "लंबवत पलटें"},

    # ============================================================
    # HERRAMIENTAS (nombres)
    # ============================================================
    "Lápiz": {"en": "Pencil", "fr": "Crayon", "de": "Bleistift", "it": "Matita", "pt": "Lápis", "zh": "铅笔", "ar": "قلم رصاص", "ru": "Карандаш", "ja": "鉛筆", "ko": "연필", "tr": "Kalem", "pl": "Ołówek", "nl": "Potlood", "sv": "Penna", "hi": "पेंसिल"},
    "Goma": {"en": "Eraser", "fr": "Gomme", "de": "Radiergummi", "it": "Gomma", "pt": "Borracha", "zh": "橡皮擦", "ar": "ممحاة", "ru": "Ластик", "ja": "消しゴム", "ko": "지우개", "tr": "Silgi", "pl": "Gumka", "nl": "Gum", "sv": "Suddgummi", "hi": "इरेज़र"},
    "Cubeta": {"en": "Bucket", "fr": "Pot de peinture", "de": "Farbeimer", "it": "Secchio", "pt": "Balde", "zh": "油漆桶", "ar": "دلو", "ru": "Заливка", "ja": "塗りつぶし", "ko": "채우기", "tr": "Kova", "pl": "Wiadro", "nl": "Emmer", "sv": "Hink", "hi": "बाल्टी"},
    "Gotero": {"en": "Picker", "fr": "Pipette", "de": "Pipette", "it": "Contagocce", "pt": "Conta-gotas", "zh": "吸管", "ar": "قطارة", "ru": "Пипетка", "ja": "スポイト", "ko": "스포이드", "tr": "Damlalık", "pl": "Kroplomierz", "nl": "Pipet", "sv": "Pipett", "hi": "पिकर"},
    "Spray": {"en": "Spray", "fr": "Aérosol", "de": "Sprühdose", "it": "Spray", "pt": "Spray", "zh": "喷枪", "ar": "بخاخ", "ru": "Распылитель", "ja": "スプレー", "ko": "스프레이", "tr": "Sprey", "pl": "Spray", "nl": "Spray", "sv": "Spray", "hi": "स्प्रे"},
    "Texto": {"en": "Text", "fr": "Texte", "de": "Text", "it": "Testo", "pt": "Texto", "zh": "文本", "ar": "نص", "ru": "Текст", "ja": "テキスト", "ko": "텍스트", "tr": "Metin", "pl": "Tekst", "nl": "Tekst", "sv": "Text", "hi": "पाठ"},
    "Selección": {"en": "Selection", "fr": "Sélection", "de": "Auswahl", "it": "Selezione", "pt": "Seleção", "zh": "选择", "ar": "تحديد", "ru": "Выделение", "ja": "選択", "ko": "선택", "tr": "Seçim", "pl": "Zaznaczenie", "nl": "Selectie", "sv": "Markering", "hi": "चयन"},
    "Selección libre": {"en": "Free selection", "fr": "Sélection libre", "de": "Freie Auswahl", "it": "Selezione libera", "pt": "Seleção livre", "zh": "自由选择", "ar": "تحديد حر", "ru": "Свободное выделение", "ja": "自由選択", "ko": "자유 선택", "tr": "Serbest seçim", "pl": "Swobodne zaznaczenie", "nl": "Vrije selectie", "sv": "Fri markering", "hi": "मुक्त चयन"},
    "Línea": {"en": "Line", "fr": "Ligne", "de": "Linie", "it": "Linea", "pt": "Linha", "zh": "直线", "ar": "خط", "ru": "Линия", "ja": "線", "ko": "선", "tr": "Çizgi", "pl": "Linia", "nl": "Lijn", "sv": "Linje", "hi": "रेखा"},
    "Rectángulo": {"en": "Rectangle", "fr": "Rectangle", "de": "Rechteck", "it": "Rettangolo", "pt": "Retângulo", "zh": "矩形", "ar": "مستطيل", "ru": "Прямоугольник", "ja": "長方形", "ko": "사각형", "tr": "Dikdörtgen", "pl": "Prostokąt", "nl": "Rechthoek", "sv": "Rektangel", "hi": "आयत"},
    "Elipse": {"en": "Ellipse", "fr": "Ellipse", "de": "Ellipse", "it": "Ellisse", "pt": "Elipse", "zh": "椭圆", "ar": "بيضاوي", "ru": "Эллипс", "ja": "楕円", "ko": "타원", "tr": "Elips", "pl": "Elipsa", "nl": "Ellips", "sv": "Ellips", "hi": "दीर्घवृत्त"},
    "Rect. redondeado": {"en": "Rounded rect", "fr": "Rectangle arrondi", "de": "Abgerundetes Rechteck", "it": "Rettangolo arrotondato", "pt": "Retângulo arredondado", "zh": "圆角矩形", "ar": "مستطيل مستدير", "ru": "Скруглённый прямоугольник", "ja": "角丸長方形", "ko": "둥근 사각형", "tr": "Yuvarlak dikdörtgen", "pl": "Zaokrąglony prostokąt", "nl": "Afgeronde rechthoek", "sv": "Rundad rektangel", "hi": "गोल आयत"},
    "Triángulo": {"en": "Triangle", "fr": "Triangle", "de": "Dreieck", "it": "Triangolo", "pt": "Triângulo", "zh": "三角形", "ar": "مثلث", "ru": "Треугольник", "ja": "三角形", "ko": "삼각형", "tr": "Üçgen", "pl": "Trójkąt", "nl": "Driehoek", "sv": "Triangel", "hi": "त्रिभुज"},
    "Triángulo rect.": {"en": "Right triangle", "fr": "Triangle rectangle", "de": "Rechtwinkliges Dreieck", "it": "Triangolo rettangolo", "pt": "Triângulo retângulo", "zh": "直角三角形", "ar": "مثلث قائم", "ru": "Прямоугольный треугольник", "ja": "直角三角形", "ko": "직각삼각형", "tr": "Dik üçgen", "pl": "Trójkąt prostokątny", "nl": "Rechthoekige driehoek", "sv": "Rätvinklig triangel", "hi": "समकोण त्रिभुज"},
    "Rombo": {"en": "Diamond", "fr": "Losange", "de": "Raute", "it": "Rombo", "pt": "Losango", "zh": "菱形", "ar": "معين", "ru": "Ромб", "ja": "ひし形", "ko": "마름모", "tr": "Eşkenar dörtgen", "pl": "Romb", "nl": "Ruit", "sv": "Romb", "hi": "समचतुर्भुज"},
    "Pentágono": {"en": "Pentagon", "fr": "Pentagone", "de": "Fünfeck", "it": "Pentagono", "pt": "Pentágono", "zh": "五边形", "ar": "خماسي", "ru": "Пятиугольник", "ja": "五角形", "ko": "오각형", "tr": "Beşgen", "pl": "Pięciokąt", "nl": "Vijfhoek", "sv": "Femhörning", "hi": "पंचभुज"},
    "Hexágono": {"en": "Hexagon", "fr": "Hexagone", "de": "Sechseck", "it": "Esagono", "pt": "Hexágono", "zh": "六边形", "ar": "سداسي", "ru": "Шестиугольник", "ja": "六角形", "ko": "육각형", "tr": "Altıgen", "pl": "Sześciokąt", "nl": "Zeshoek", "sv": "Sexhörning", "hi": "षट्भुज"},
    "Flecha derecha": {"en": "Right arrow", "fr": "Flèche droite", "de": "Pfeil rechts", "it": "Freccia destra", "pt": "Seta direita", "zh": "右箭头", "ar": "سهم يمين", "ru": "Стрелка вправо", "ja": "右矢印", "ko": "오른쪽 화살표", "tr": "Sağ ok", "pl": "Strzałka w prawo", "nl": "Pijl rechts", "sv": "Högerpil", "hi": "दाएँ तीर"},
    "Flecha izquierda": {"en": "Left arrow", "fr": "Flèche gauche", "de": "Pfeil links", "it": "Freccia sinistra", "pt": "Seta esquerda", "zh": "左箭头", "ar": "سهم يسار", "ru": "Стрелка влево", "ja": "左矢印", "ko": "왼쪽 화살표", "tr": "Sol ok", "pl": "Strzałka w lewo", "nl": "Pijl links", "sv": "Vänsterpil", "hi": "बाएँ तीर"},
    "Estrella": {"en": "Star", "fr": "Étoile", "de": "Stern", "it": "Stella", "pt": "Estrela", "zh": "星形", "ar": "نجمة", "ru": "Звезда", "ja": "星", "ko": "별", "tr": "Yıldız", "pl": "Gwiazda", "nl": "Ster", "sv": "Stjärna", "hi": "तारा"},
    "Corazón": {"en": "Heart", "fr": "Cœur", "de": "Herz", "it": "Cuore", "pt": "Coração", "zh": "心形", "ar": "قلب", "ru": "Сердце", "ja": "ハート", "ko": "하트", "tr": "Kalp", "pl": "Serce", "nl": "Hart", "sv": "Hjärta", "hi": "दिल"},
    "Cubo": {"en": "Cube", "fr": "Cube", "de": "Würfel", "it": "Cubo", "pt": "Cubo", "zh": "立方体", "ar": "مكعب", "ru": "Куб", "ja": "立方体", "ko": "큐브", "tr": "Küp", "pl": "Sześcian", "nl": "Kubus", "sv": "Kub", "hi": "घन"},
    "Zoom": {"en": "Zoom", "fr": "Zoom", "de": "Zoom", "it": "Zoom", "pt": "Zoom", "zh": "缩放", "ar": "تكبير", "ru": "Масштаб", "ja": "ズーム", "ko": "줌", "tr": "Yakınlaştırma", "pl": "Powiększenie", "nl": "Zoom", "sv": "Zoom", "hi": "ज़ूम"},
    "Pincel": {"en": "Brush", "fr": "Pinceau", "de": "Pinsel", "it": "Pennello", "pt": "Pincel", "zh": "画笔", "ar": "فرشاة", "ru": "Кисть", "ja": "ブラシ", "ko": "브러시", "tr": "Fırça", "pl": "Pędzel", "nl": "Penseel", "sv": "Pensel", "hi": "ब्रश"},
    "Pincel personalizado": {"en": "Custom brush", "fr": "Pinceau personnalisé", "de": "Benutzerdefinierter Pinsel", "it": "Pennello personalizzato", "pt": "Pincel personalizado", "zh": "自定义画笔", "ar": "فرشاة مخصصة", "ru": "Пользовательская кисть", "ja": "カスタムブラシ", "ko": "사용자 정의 브러시", "tr": "Özel fırça", "pl": "Niestandardowy pędzel", "nl": "Aangepast penseel", "sv": "Anpassad pensel", "hi": "कस्टम ब्रश"},
    "Crayón": {"en": "Crayon", "fr": "Crayon de cire", "de": "Wachsmalstift", "it": "Pastello a cera", "pt": "Giz de cera", "zh": "蜡笔", "ar": "طبشور شمعي", "ru": "Восковой мелок", "ja": "クレヨン", "ko": "크레용", "tr": "Mum boya", "pl": "Kredka", "nl": "Krijt", "sv": "Krita", "hi": "क्रेयॉन"},
    "Marcador": {"en": "Marker", "fr": "Marqueur", "de": "Marker", "it": "Marcatore", "pt": "Marcador", "zh": "马克笔", "ar": "قلم تحديد", "ru": "Маркер", "ja": "マーカー", "ko": "마커", "tr": "İşaretleyici", "pl": "Marker", "nl": "Markeerstift", "sv": "Markör", "hi": "मार्कर"},
    "Acuarela": {"en": "Watercolor", "fr": "Aquarelle", "de": "Aquarell", "it": "Acquerello", "pt": "Aquarela", "zh": "水彩", "ar": "ألوان مائية", "ru": "Акварель", "ja": "水彩", "ko": "수채화", "tr": "Sulu boya", "pl": "Akwarela", "nl": "Aquarel", "sv": "Akvarell", "hi": "जलरंग"},
    "Óleo": {"en": "Oil paint", "fr": "Peinture à l'huile", "de": "Ölfarbe", "it": "Olio", "pt": "Óleo", "zh": "油画", "ar": "رسم زيتي", "ru": "Масло", "ja": "油絵", "ko": "유화", "tr": "Yağlı boya", "pl": "Olej", "nl": "Olieverf", "sv": "Oljefärg", "hi": "तैल चित्र"},
    "Caligrafía": {"en": "Calligraphy", "fr": "Calligraphie", "de": "Kalligraphie", "it": "Calligrafia", "pt": "Caligrafia", "zh": "书法", "ar": "خط", "ru": "Каллиграфия", "ja": "カリグラフィ", "ko": "서예", "tr": "Kaligrafi", "pl": "Kaligrafia", "nl": "Kalligrafie", "sv": "Kalligrafi", "hi": "सुलेख"},
    "Resaltador": {"en": "Highlighter", "fr": "Surligneur", "de": "Textmarker", "it": "Evidenziatore", "pt": "Marcador de texto", "zh": "荧光笔", "ar": "قلم تمييز", "ru": "Маркер", "ja": "ハイライター", "ko": "형광펜", "tr": "Fosforlu kalem", "pl": "Zakreślacz", "nl": "Markeerstift", "sv": "Överstrykningspenna", "hi": "हाइलाइटर"},
    "Espejo": {"en": "Mirror", "fr": "Miroir", "de": "Spiegel", "it": "Specchio", "pt": "Espelho", "zh": "镜像", "ar": "مرآة", "ru": "Зеркало", "ja": "ミラー", "ko": "미러", "tr": "Ayna", "pl": "Lustro", "nl": "Spiegel", "sv": "Spegel", "hi": "दर्पण"},
    "Trazo Pixel": {"en": "Pixel stroke", "fr": "Trait pixel", "de": "Pixelstrich", "it": "Tratto pixel", "pt": "Traço pixel", "zh": "像素笔触", "ar": "خط بكسل", "ru": "Пиксельный штрих", "ja": "ピクセルストローク", "ko": "픽셀 스트로크", "tr": "Piksel çizgisi", "pl": "Kreska pikselowa", "nl": "Pixelstreek", "sv": "Pixelstreck", "hi": "पिक्सेल स्ट्रोक"},
    "Varita mágica": {"en": "Magic wand", "fr": "Baguette magique", "de": "Zauberstab", "it": "Bacchetta magica", "pt": "Varinha mágica", "zh": "魔棒", "ar": "عصا سحرية", "ru": "Волшебная палочка", "ja": "魔法の杖", "ko": "마법봉", "tr": "Sihirli değnek", "pl": "Różdżka", "nl": "Toverstaf", "sv": "Trollspö", "hi": "जादू की छड़ी"},
    "Recortar dentro": {"en": "Crop inside", "fr": "Recadrer à l'intérieur", "de": "Innen zuschneiden", "it": "Ritaglia dentro", "pt": "Cortar dentro", "zh": "内部裁剪", "ar": "اقتصاص الداخل", "ru": "Обрезать внутри", "ja": "内側をトリミング", "ko": "안쪽 자르기", "tr": "İçten kırp", "pl": "Przytnij wewnątrz", "nl": "Binnen bijsnijden", "sv": "Beskär inuti", "hi": "अंदर क्रॉप करें"},
    "Recortar fuera": {"en": "Crop outside", "fr": "Recadrer à l'extérieur", "de": "Außen zuschneiden", "it": "Ritaglia fuori", "pt": "Cortar fora", "zh": "外部裁剪", "ar": "اقتصاص الخارج", "ru": "Обрезать снаружи", "ja": "外側をトリミング", "ko": "바깥쪽 자르기", "tr": "Dıştan kırp", "pl": "Przytnij na zewnątrz", "nl": "Buiten bijsnijden", "sv": "Beskär utanför", "hi": "बाहर क्रॉप करें"},
    "Desenfoque": {"en": "Blur", "fr": "Flou", "de": "Weichzeichnen", "it": "Sfocatura", "pt": "Desfoque", "zh": "模糊", "ar": "ضبابية", "ru": "Размытие", "ja": "ぼかし", "ko": "흐림", "tr": "Bulanıklık", "pl": "Rozmycie", "nl": "Vervagen", "sv": "Oskärpa", "hi": "धुंधलापन"},
    "Corrector": {"en": "Heal", "fr": "Correcteur", "de": "Bereinigen", "it": "Correttore", "pt": "Correção", "zh": "修复", "ar": "معالجة", "ru": "Восстановление", "ja": "修復", "ko": "치유", "tr": "İyileştir", "pl": "Leczenie", "nl": "Herstellen", "sv": "Laga", "hi": "हील"},
    "Subexponer": {"en": "Burn", "fr": "Densité +", "de": "Nachbelichten", "it": "Brucia", "pt": "Subexpor", "zh": "加深", "ar": "حرق", "ru": "Затемнение", "ja": "焼き込み", "ko": "번", "tr": "Yak", "pl": "Przypal", "nl": "Tegenhouden", "sv": "Efterbelys", "hi": "बर्न"},
    "Gradiente": {"en": "Gradient", "fr": "Dégradé", "de": "Verlauf", "it": "Sfumatura", "pt": "Gradiente", "zh": "渐变", "ar": "تدرج", "ru": "Градиент", "ja": "グラデーション", "ko": "그라디언트", "tr": "Gradyan", "pl": "Gradient", "nl": "Verloop", "sv": "Gradient", "hi": "ग्रेडिएंट"},
    "Clonar": {"en": "Clone", "fr": "Cloner", "de": "Klonen", "it": "Clona", "pt": "Clonar", "zh": "克隆", "ar": "استنساخ", "ru": "Клонирование", "ja": "複製", "ko": "복제", "tr": "Klonla", "pl": "Klonuj", "nl": "Klonen", "sv": "Klona", "hi": "क्लोन"},
    "Mover": {"en": "Move", "fr": "Déplacer", "de": "Bewegen", "it": "Sposta", "pt": "Mover", "zh": "移动", "ar": "تحريك", "ru": "Переместить", "ja": "移動", "ko": "이동", "tr": "Taşı", "pl": "Przenieś", "nl": "Verplaatsen", "sv": "Flytta", "hi": "स्थानांतरित करें"},
    "Deformar": {"en": "Deform", "fr": "Déformer", "de": "Verformen", "it": "Deforma", "pt": "Deformar", "zh": "变形", "ar": "تشويه", "ru": "Деформация", "ja": "変形", "ko": "변형", "tr": "Şekil boz", "pl": "Zniekształć", "nl": "Vervormen", "sv": "Deformera", "hi": "विकृत करें"},
    "Aclarar": {"en": "Lighten", "fr": "Éclaircir", "de": "Aufhellen", "it": "Schiarisci", "pt": "Clarear", "zh": "变亮", "ar": "تفتيح", "ru": "Осветление", "ja": "明るく", "ko": "밝게", "tr": "Aydınlat", "pl": "Rozjaśnij", "nl": "Lichter", "sv": "Ljusare", "hi": "हल्का"},

    # ============================================================
    # MENSAJES Y ESTADOS
    # ============================================================
    "Guardar cambios": {"en": "Save changes", "fr": "Enregistrer les modifications", "de": "Änderungen speichern", "it": "Salva modifiche", "pt": "Salvar alterações", "zh": "保存更改", "ar": "حفظ التغييرات", "ru": "Сохранить изменения", "ja": "変更を保存", "ko": "변경 사항 저장", "tr": "Değişiklikleri kaydet", "pl": "Zapisz zmiany", "nl": "Wijzigingen opslaan", "sv": "Spara ändringar", "hi": "परिवर्तन सहेजें"},
    "Deseas guardar el lienzo actual?": {"en": "Do you want to save the current canvas?", "fr": "Voulez-vous enregistrer le canevas actuel ?", "de": "Möchten Sie die aktuelle Leinwand speichern?", "it": "Vuoi salvare la tela attuale?", "pt": "Deseja salvar a tela atual?", "zh": "要保存当前画布吗？", "ar": "هل تريد حفظ اللوحة الحالية؟", "ru": "Сохранить текущий холст?", "ja": "現在のキャンバスを保存しますか？", "ko": "현재 캔버스를 저장하시겠습니까?", "tr": "Geçerli tuvali kaydetmek istiyor musunuz?", "pl": "Czy chcesz zapisać bieżące płótno?", "nl": "Wilt u het huidige canvas opslaan?", "sv": "Vill du spara den aktuella duken?", "hi": "क्या आप वर्तमान कैनवास सहेजना चाहते हैं?"},
    "Capa bloqueada": {"en": "Locked layer", "fr": "Calque verrouillé", "de": "Ebene gesperrt", "it": "Livello bloccato", "pt": "Camada bloqueada", "zh": "图层已锁定", "ar": "الطبقة مقفلة", "ru": "Слой заблокирован", "ja": "レイヤーがロックされています", "ko": "레이어 잠김", "tr": "Kilitli katman", "pl": "Warstwa zablokowana", "nl": "Laag vergrendeld", "sv": "Lager låst", "hi": "परत लॉक है"},
    "Error": {"en": "Error", "fr": "Erreur", "de": "Fehler", "it": "Errore", "pt": "Erro", "zh": "错误", "ar": "خطأ", "ru": "Ошибка", "ja": "エラー", "ko": "오류", "tr": "Hata", "pl": "Błąd", "nl": "Fout", "sv": "Fel", "hi": "त्रुटि"},
    "Filtros y Color": {"en": "Filters and Color", "fr": "Filtres et couleur", "de": "Filter und Farbe", "it": "Filtri e colore", "pt": "Filtros e cor", "zh": "滤镜和颜色", "ar": "الفلاتر واللون", "ru": "Фильтры и цвет", "ja": "フィルターと色", "ko": "필터 및 색상", "tr": "Filtreler ve Renk", "pl": "Filtry i kolor", "nl": "Filters en kleur", "sv": "Filter och färg", "hi": "फ़िल्टर और रंग"},
    "Configurar Pincel Personal": {"en": "Configure Custom Brush", "fr": "Configurer le pinceau personnalisé", "de": "Benutzerdefinierten Pinsel konfigurieren", "it": "Configura pennello personalizzato", "pt": "Configurar pincel personalizado", "zh": "配置自定义画笔", "ar": "تكوين الفرشاة المخصصة", "ru": "Настроить пользовательскую кисть", "ja": "カスタムブラシを設定", "ko": "사용자 정의 브러시 구성", "tr": "Özel fırçayı yapılandır", "pl": "Skonfiguruj niestandardowy pędzel", "nl": "Aangepast penseel configureren", "sv": "Konfigurera anpassad pensel", "hi": "कस्टम ब्रश कॉन्फ़िगर करें"},
    "Tamano / Recorte": {"en": "Size / Crop", "fr": "Taille / Recadrage", "de": "Größe / Zuschneiden", "it": "Dimensione / Ritaglio", "pt": "Tamanho / Recorte", "zh": "大小 / 裁剪", "ar": "الحجم / القص", "ru": "Размер / Обрезка", "ja": "サイズ / トリミング", "ko": "크기 / 자르기", "tr": "Boyut / Kırpma", "pl": "Rozmiar / Kadrowanie", "nl": "Grootte / Bijsnijden", "sv": "Storlek / Beskär", "hi": "आकार / क्रॉप"},
    "Modo Oscuro": {"en": "Dark Mode", "fr": "Mode sombre", "de": "Dunkler Modus", "it": "Modalità scura", "pt": "Modo escuro", "zh": "深色模式", "ar": "الوضع الداكن", "ru": "Тёмный режим", "ja": "ダークモード", "ko": "다크 모드", "tr": "Karanlık mod", "pl": "Tryb ciemny", "nl": "Donkere modus", "sv": "Mörkt läge", "hi": "डार्क मोड"},
    "Modo Claro": {"en": "Light Mode", "fr": "Mode clair", "de": "Heller Modus", "it": "Modalità chiara", "pt": "Modo claro", "zh": "浅色模式", "ar": "الوضع الفاتح", "ru": "Светлый режим", "ja": "ライトモード", "ko": "라이트 모드", "tr": "Aydınlık mod", "pl": "Tryb jasny", "nl": "Lichte modus", "sv": "Ljust läge", "hi": "लाइट मोड"},

    # ============================================================
    # MENSAJES DE ARCHIVO
    # ============================================================
    "Reinicio Requerido": {"en": "Restart Required", "fr": "Redémarrage requis", "de": "Neustart erforderlich", "it": "Riavvio necessario", "pt": "Reinicialização necessária", "zh": "需要重启", "ar": "إعادة التشغيل مطلوبة", "ru": "Требуется перезапуск", "ja": "再起動が必要", "ko": "다시 시작 필요", "tr": "Yeniden başlatma gerekli", "pl": "Wymagane ponowne uruchomienie", "nl": "Opnieuw starten vereist", "sv": "Omstart krävs", "hi": "पुनः आरंभ आवश्यक"},
    "La aplicacion se reiniciara para cambiar el idioma.": {"en": "The application will restart to change the language.", "fr": "L'application va redémarrer pour changer la langue.", "de": "Die Anwendung wird neu gestartet, um die Sprache zu ändern.", "it": "L'applicazione si riavvierà per cambiare la lingua.", "pt": "A aplicação será reiniciada para alterar o idioma.", "zh": "应用程序将重启以更改语言。", "ar": "سيتم إعادة تشغيل التطبيق لتغيير اللغة.", "ru": "Приложение будет перезапущено для смены языка.", "ja": "言語を変更するためにアプリケーションが再起動します。", "ko": "언어를 변경하기 위해 응용 프로그램이 다시 시작됩니다.", "tr": "Dili değiştirmek için uygulama yeniden başlatılacak.", "pl": "Aplikacja zostanie uruchomiona ponownie, aby zmienić język.", "nl": "De applicatie wordt opnieuw gestart om de taal te wijzigen.", "sv": "Applikationen startas om för att ändra språket.", "hi": "भाषा बदलने के लिए एप्लिकेशन पुनः आरंभ होगा।"},
    "Guardar como": {"en": "Save as", "fr": "Enregistrer sous", "de": "Speichern unter", "it": "Salva come", "pt": "Salvar como", "zh": "另存为", "ar": "حفظ باسم", "ru": "Сохранить как", "ja": "名前を付けて保存", "ko": "다른 이름으로 저장", "tr": "Farklı kaydet", "pl": "Zapisz jako", "nl": "Opslaan als", "sv": "Spara som", "hi": "इस रूप में सहेजें"},
    "Imagenes (*.png *.jpg *.jpeg *.bmp)": {"en": "Images (*.png *.jpg *.jpeg *.bmp)", "fr": "Images (*.png *.jpg *.jpeg *.bmp)", "de": "Bilder (*.png *.jpg *.jpeg *.bmp)", "it": "Immagini (*.png *.jpg *.jpeg *.bmp)", "pt": "Imagens (*.png *.jpg *.jpeg *.bmp)", "zh": "图像 (*.png *.jpg *.jpeg *.bmp)", "ar": "الصور (*.png *.jpg *.jpeg *.bmp)", "ru": "Изображения (*.png *.jpg *.jpeg *.bmp)", "ja": "画像 (*.png *.jpg *.jpeg *.bmp)", "ko": "이미지 (*.png *.jpg *.jpeg *.bmp)", "tr": "Resimler (*.png *.jpg *.jpeg *.bmp)", "pl": "Obrazy (*.png *.jpg *.jpeg *.bmp)", "nl": "Afbeeldingen (*.png *.jpg *.jpeg *.bmp)", "sv": "Bilder (*.png *.jpg *.jpeg *.bmp)", "hi": "छवियाँ (*.png *.jpg *.jpeg *.bmp)"},
    "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG Vectorial (*.svg)": {"en": "PNG (*.png);;JPEG (*.jpg *.jpeg);;Vector SVG (*.svg)", "fr": "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG vectoriel (*.svg)", "de": "PNG (*.png);;JPEG (*.jpg *.jpeg);;Vektor-SVG (*.svg)", "it": "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG vettoriale (*.svg)", "pt": "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG vetorial (*.svg)", "zh": "PNG (*.png);;JPEG (*.jpg *.jpeg);;矢量 SVG (*.svg)", "ar": "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG متجهي (*.svg)", "ru": "PNG (*.png);;JPEG (*.jpg *.jpeg);;Векторный SVG (*.svg)", "ja": "PNG (*.png);;JPEG (*.jpg *.jpeg);;ベクター SVG (*.svg)", "ko": "PNG (*.png);;JPEG (*.jpg *.jpeg);;벡터 SVG (*.svg)", "tr": "PNG (*.png);;JPEG (*.jpg *.jpeg);;Vektör SVG (*.svg)", "pl": "PNG (*.png);;JPEG (*.jpg *.jpeg);;SVG wektorowe (*.svg)", "nl": "PNG (*.png);;JPEG (*.jpg *.jpeg);;Vector-SVG (*.svg)", "sv": "PNG (*.png);;JPEG (*.jpg *.jpeg);;Vektor-SVG (*.svg)", "hi": "PNG (*.png);;JPEG (*.jpg *.jpeg);;वेक्टर SVG (*.svg)"},
    "Imagen cargada.": {"en": "Image loaded.", "fr": "Image chargée.", "de": "Bild geladen.", "it": "Immagine caricata.", "pt": "Imagem carregada.", "zh": "图像已加载。", "ar": "تم تحميل الصورة.", "ru": "Изображение загружено.", "ja": "画像が読み込まれました。", "ko": "이미지 로드됨.", "tr": "Resim yüklendi.", "pl": "Obraz załadowany.", "nl": "Afbeelding geladen.", "sv": "Bild laddad.", "hi": "छवि लोड की गई।"},
    "Imagen guardada: %1": {"en": "Image saved: %1", "fr": "Image enregistrée : %1", "de": "Bild gespeichert: %1", "it": "Immagine salvata: %1", "pt": "Imagem salva: %1", "zh": "图像已保存：%1", "ar": "تم حفظ الصورة: %1", "ru": "Изображение сохранено: %1", "ja": "画像を保存しました: %1", "ko": "이미지 저장됨: %1", "tr": "Resim kaydedildi: %1", "pl": "Zapisano obraz: %1", "nl": "Afbeelding opgeslagen: %1", "sv": "Bild sparad: %1", "hi": "छवि सहेजी गई: %1"},
    "No se pudo cargar la imagen.": {"en": "Could not load the image.", "fr": "Impossible de charger l'image.", "de": "Das Bild konnte nicht geladen werden.", "it": "Impossibile caricare l'immagine.", "pt": "Não foi possível carregar a imagem.", "zh": "无法加载图像。", "ar": "تعذر تحميل الصورة.", "ru": "Не удалось загрузить изображение.", "ja": "画像を読み込めませんでした。", "ko": "이미지를 로드할 수 없습니다.", "tr": "Resim yüklenemedi.", "pl": "Nie można załadować obrazu.", "nl": "Kan de afbeelding niet laden.", "sv": "Kunde inte ladda bilden.", "hi": "छवि लोड नहीं की जा सकी।"},
    "No se pudo guardar la imagen.": {"en": "Could not save the image.", "fr": "Impossible d'enregistrer l'image.", "de": "Das Bild konnte nicht gespeichert werden.", "it": "Impossibile salvare l'immagine.", "pt": "Não foi possível salvar a imagem.", "zh": "无法保存图像。", "ar": "تعذر حفظ الصورة.", "ru": "Не удалось сохранить изображение.", "ja": "画像を保存できませんでした。", "ko": "이미지를 저장할 수 없습니다.", "tr": "Resim kaydedilemedi.", "pl": "Nie można zapisać obrazu.", "nl": "Kan de afbeelding niet opslaan.", "sv": "Kunde inte spara bilden.", "hi": "छवि सहेजी नहीं जा सकी।"},
    "No se pudo guardar el archivo SVG.": {"en": "Could not save the SVG file.", "fr": "Impossible d'enregistrer le fichier SVG.", "de": "Die SVG-Datei konnte nicht gespeichert werden.", "it": "Impossibile salvare il file SVG.", "pt": "Não foi possível salvar o arquivo SVG.", "zh": "无法保存 SVG 文件。", "ar": "تعذر حفظ ملف SVG.", "ru": "Не удалось сохранить файл SVG.", "ja": "SVG ファイルを保存できませんでした。", "ko": "SVG 파일을 저장할 수 없습니다.", "tr": "SVG dosyası kaydedilemedi.", "pl": "Nie można zapisać pliku SVG.", "nl": "Kan het SVG-bestand niet opslaan.", "sv": "Kunde inte spara SVG-filen.", "hi": "SVG फ़ाइल सहेजी नहीं जा सकी।"},
    "Imagen insertada como nueva capa.": {"en": "Image inserted as new layer.", "fr": "Image insérée comme nouveau calque.", "de": "Bild als neue Ebene eingefügt.", "it": "Immagine inserita come nuovo livello.", "pt": "Imagem inserida como nova camada.", "zh": "图像已作为新图层插入。", "ar": "تم إدراج الصورة كطبقة جديدة.", "ru": "Изображение вставлено как новый слой.", "ja": "画像が新規レイヤーとして挿入されました。", "ko": "이미지가 새 레이어로 삽입되었습니다.", "tr": "Resim yeni katman olarak eklendi.", "pl": "Obraz wstawiony jako nowa warstwa.", "nl": "Afbeelding ingevoegd als nieuwe laag.", "sv": "Bild infogad som nytt lager.", "hi": "छवि नई परत के रूप में सम्मिलित की गई।"},
    "Filtros aplicados exitosamente": {"en": "Filters applied successfully", "fr": "Filtres appliqués avec succès", "de": "Filter erfolgreich angewendet", "it": "Filtri applicati con successo", "pt": "Filtros aplicados com sucesso", "zh": "滤镜应用成功", "ar": "تم تطبيق الفلاتر بنجاح", "ru": "Фильтры успешно применены", "ja": "フィルターが正常に適用されました", "ko": "필터가 성공적으로 적용되었습니다", "tr": "Filtreler başarıyla uygulandı", "pl": "Filtry zastosowane pomyślnie", "nl": "Filters succesvol toegepast", "sv": "Filter tillämpades framgångsrikt", "hi": "फ़िल्टर सफलतापूर्वक लागू किए गए"},

    # ============================================================
    # MENSAJES DE MÁSCARAS
    # ============================================================
    "Máscara: NEGRO oculta · BLANCO revela · Goma revela · X intercambia": {"en": "Mask: BLACK hides · WHITE reveals · Eraser reveals · X swaps", "fr": "Masque : NOIR cache · BLANC révèle · Gomme révèle · X échange", "de": "Maske: SCHWARZ verdeckt · WEISS zeigt · Radierer zeigt · X tauscht", "it": "Maschera: NERO nasconde · BIANCO rivela · Gomma rivela · X scambia", "pt": "Máscara: PRETO oculta · BRANCO revela · Borracha revela · X troca", "zh": "蒙版：黑色隐藏 · 白色显示 · 橡皮擦显示 · X 交换", "ar": "القناع: الأسود يخفي · الأبيض يكشف · الممحاة تكشف · X يبدل", "ru": "Маска: ЧЁРНЫЙ скрывает · БЕЛЫЙ показывает · Ластик показывает · X меняет", "ja": "マスク：黒は隠す · 白は表示 · 消しゴムは表示 · X は入れ替え", "ko": "마스크: 검정 숨김 · 흰색 표시 · 지우개 표시 · X 교환", "tr": "Maske: SİYAH gizler · BEYAZ gösterir · Silgi gösterir · X değiştirir", "pl": "Maska: CZARNY ukrywa · BIAŁY pokazuje · Gumka pokazuje · X zamienia", "nl": "Masker: ZWART verbergt · WIT toont · Gum toont · X wisselt", "sv": "Mask: SVART döljer · VIT visar · Suddgummi visar · X byter", "hi": "मास्क: काला छिपाता है · सफ़ेद दिखाता है · इरेज़र दिखाता है · X बदलता है"},
    "Máscara eliminada": {"en": "Mask deleted", "fr": "Masque supprimé", "de": "Maske gelöscht", "it": "Maschera eliminata", "pt": "Máscara excluída", "zh": "蒙版已删除", "ar": "تم حذف القناع", "ru": "Маска удалена", "ja": "マスクを削除しました", "ko": "마스크 삭제됨", "tr": "Maske silindi", "pl": "Maska usunięta", "nl": "Masker verwijderd", "sv": "Mask borttagen", "hi": "मास्क हटा दिया गया"},
    "Máscara activada": {"en": "Mask enabled", "fr": "Masque activé", "de": "Maske aktiviert", "it": "Maschera attivata", "pt": "Máscara ativada", "zh": "蒙版已启用", "ar": "تم تفعيل القناع", "ru": "Маска включена", "ja": "マスクが有効になりました", "ko": "마스크 활성화됨", "tr": "Maske etkinleştirildi", "pl": "Maska włączona", "nl": "Masker ingeschakeld", "sv": "Mask aktiverad", "hi": "मास्क सक्षम"},
    "Máscara desactivada": {"en": "Mask disabled", "fr": "Masque désactivé", "de": "Maske deaktiviert", "it": "Maschera disattivata", "pt": "Máscara desativada", "zh": "蒙版已禁用", "ar": "تم تعطيل القناع", "ru": "Маска отключена", "ja": "マスクが無効になりました", "ko": "마스크 비활성화됨", "tr": "Maske devre dışı bırakıldı", "pl": "Maska wyłączona", "nl": "Masker uitgeschakeld", "sv": "Mask inaktiverad", "hi": "मास्क अक्षम"},
    "Editando MÁSCARA: negro oculta / blanco revela": {"en": "Editing MASK: black hides / white reveals", "fr": "Édition du MASQUE : noir cache / blanc révèle", "de": "MASKE bearbeiten: Schwarz verdeckt / Weiß zeigt", "it": "Modifica MASCHERA: nero nasconde / bianco rivela", "pt": "Editando MÁSCARA: preto oculta / branco revela", "zh": "编辑蒙版：黑色隐藏 / 白色显示", "ar": "تحرير القناع: الأسود يخفي / الأبيض يكشف", "ru": "Редактирование МАСКИ: чёрный скрывает / белый показывает", "ja": "マスクを編集中：黒は隠す / 白は表示", "ko": "마스크 편집 중: 검정 숨김 / 흰색 표시", "tr": "MASKE düzenleniyor: siyah gizler / beyaz gösterir", "pl": "Edytowanie MASKI: czarny ukrywa / biały pokazuje", "nl": "MASKER bewerken: zwart verbergt / wit toont", "sv": "Redigerar MASK: svart döljer / vit visar", "hi": "मास्क संपादित: काला छिपाता है / सफ़ेद दिखाता है"},
    "Máscara aplicada a la capa": {"en": "Mask applied to layer", "fr": "Masque appliqué au calque", "de": "Maske auf Ebene angewendet", "it": "Maschera applicata al livello", "pt": "Máscara aplicada à camada", "zh": "蒙版已应用到图层", "ar": "تم تطبيق القناع على الطبقة", "ru": "Маска применена к слою", "ja": "マスクをレイヤーに適用しました", "ko": "레이어에 마스크 적용됨", "tr": "Maske katmana uygulandı", "pl": "Maska zastosowana do warstwy", "nl": "Masker toegepast op laag", "sv": "Mask tillämpad på lager", "hi": "मास्क परत पर लागू"},
    "Máscara invertida": {"en": "Mask inverted", "fr": "Masque inversé", "de": "Maske invertiert", "it": "Maschera invertita", "pt": "Máscara invertida", "zh": "蒙版已反转", "ar": "تم عكس القناع", "ru": "Маска инвертирована", "ja": "マスクを反転しました", "ko": "마스크 반전됨", "tr": "Maske ters çevrildi", "pl": "Maska odwrócona", "nl": "Masker geïnverteerd", "sv": "Mask inverterad", "hi": "मास्क उलटा"},
    "Máscara de COLOR añadida": {"en": "COLOR mask added", "fr": "Masque de COULEUR ajouté", "de": "FARB-Maske hinzugefügt", "it": "Maschera COLORATA aggiunta", "pt": "Máscara de COR adicionada", "zh": "颜色蒙版已添加", "ar": "تمت إضافة قناع اللون", "ru": "ЦВЕТНАЯ маска добавлена", "ja": "カラーマスクを追加しました", "ko": "색상 마스크 추가됨", "tr": "RENK maskesi eklendi", "pl": "Dodano maskę KOLORU", "nl": "KLEUR-masker toegevoegd", "sv": "FÄRG-mask tillagd", "hi": "रंग मास्क जोड़ा गया"},
    "Máscara de color eliminada": {"en": "Color mask deleted", "fr": "Masque de couleur supprimé", "de": "Farbmaske gelöscht", "it": "Maschera colore eliminata", "pt": "Máscara de cor excluída", "zh": "颜色蒙版已删除", "ar": "تم حذف قناع اللون", "ru": "Цветная маска удалена", "ja": "カラーマスクを削除しました", "ko": "색상 마스크 삭제됨", "tr": "Renk maskesi silindi", "pl": "Maska koloru usunięta", "nl": "Kleurmasker verwijderd", "sv": "Färgmask borttagen", "hi": "रंग मास्क हटा दिया गया"},
    "Máscara de color ACTIVADA": {"en": "Color mask ENABLED", "fr": "Masque de couleur ACTIVÉ", "de": "Farbmaske AKTIVIERT", "it": "Maschera colore ATTIVATA", "pt": "Máscara de cor ATIVADA", "zh": "颜色蒙版已启用", "ar": "تم تفعيل قناع اللون", "ru": "Цветная маска ВКЛЮЧЕНА", "ja": "カラーマスクを有効化しました", "ko": "색상 마스크 활성화됨", "tr": "Renk maskesi ETKİNLEŞTİRİLDİ", "pl": "Maska koloru WŁĄCZONA", "nl": "Kleurmasker INGESCHAKELD", "sv": "Färgmask AKTIVERAD", "hi": "रंग मास्क सक्षम"},
    "Máscara de color DESACTIVADA": {"en": "Color mask DISABLED", "fr": "Masque de couleur DÉSACTIVÉ", "de": "Farbmaske DEAKTIVIERT", "it": "Maschera colore DISATTIVATA", "pt": "Máscara de cor DESATIVADA", "zh": "颜色蒙版已禁用", "ar": "تم تعطيل قناع اللون", "ru": "Цветная маска ОТКЛЮЧЕНА", "ja": "カラーマスクを無効化しました", "ko": "색상 마스크 비활성화됨", "tr": "Renk maskesi DEVRE DIŞI", "pl": "Maska koloru WYŁĄCZONA", "nl": "Kleurmasker UITGESCHAKELD", "sv": "Färgmask INAKTIVERAD", "hi": "रंग मास्क अक्षम"},
    "Anadir mascara de capa": {"en": "Add layer mask", "fr": "Ajouter un masque de calque", "de": "Ebenenmaske hinzufügen", "it": "Aggiungi maschera livello", "pt": "Adicionar máscara de camada", "zh": "添加图层蒙版", "ar": "إضافة قناع الطبقة", "ru": "Добавить маску слоя", "ja": "レイヤーマスクを追加", "ko": "레이어 마스크 추가", "tr": "Katman maskesi ekle", "pl": "Dodaj maskę warstwy", "nl": "Laagmasker toevoegen", "sv": "Lägg till lagermask", "hi": "परत मास्क जोड़ें"},
    "Eliminar mascara de capa": {"en": "Delete layer mask", "fr": "Supprimer le masque de calque", "de": "Ebenenmaske löschen", "it": "Elimina maschera livello", "pt": "Excluir máscara de camada", "zh": "删除图层蒙版", "ar": "حذف قناع الطبقة", "ru": "Удалить маску слоя", "ja": "レイヤーマスクを削除", "ko": "레이어 마스크 삭제", "tr": "Katman maskesini sil", "pl": "Usuń maskę warstwy", "nl": "Laagmasker verwijderen", "sv": "Ta bort lagermask", "hi": "परत मास्क हटाएँ"},
    "Desactivar mascara": {"en": "Disable mask", "fr": "Désactiver le masque", "de": "Maske deaktivieren", "it": "Disattiva maschera", "pt": "Desativar máscara", "zh": "禁用蒙版", "ar": "تعطيل القناع", "ru": "Отключить маску", "ja": "マスクを無効化", "ko": "마스크 비활성화", "tr": "Maskeyi devre dışı bırak", "pl": "Wyłącz maskę", "nl": "Masker uitschakelen", "sv": "Inaktivera mask", "hi": "मास्क अक्षम करें"},
    "Invertir mascara": {"en": "Invert mask", "fr": "Inverser le masque", "de": "Maske invertieren", "it": "Inverti maschera", "pt": "Inverter máscara", "zh": "反转蒙版", "ar": "عكس القناع", "ru": "Инвертировать маску", "ja": "マスクを反転", "ko": "마스크 반전", "tr": "Maskeyi ters çevir", "pl": "Odwróć maskę", "nl": "Masker omkeren", "sv": "Invertera mask", "hi": "मास्क उलटा करें"},
    "Aplicar mascara de capa": {"en": "Apply layer mask", "fr": "Appliquer le masque de calque", "de": "Ebenenmaske anwenden", "it": "Applica maschera livello", "pt": "Aplicar máscara de camada", "zh": "应用图层蒙版", "ar": "تطبيق قناع الطبقة", "ru": "Применить маску слоя", "ja": "レイヤーマスクを適用", "ko": "레이어 마스크 적용", "tr": "Katman maskesini uygula", "pl": "Zastosuj maskę warstwy", "nl": "Laagmasker toepassen", "sv": "Tillämpa lagermask", "hi": "परत मास्क लागू करें"},
    "Anadir MASCARA DE COLOR (filtros)...": {"en": "Add COLOR mask (filters)...", "fr": "Ajouter un masque de COULEUR (filtres)...", "de": "FARB-Maske hinzufügen (Filter)...", "it": "Aggiungi maschera COLORATA (filtri)...", "pt": "Adicionar máscara de COR (filtros)...", "zh": "添加颜色蒙版（滤镜）...", "ar": "إضافة قناع اللون (الفلاتر)...", "ru": "Добавить ЦВЕТНУЮ маску (фильтры)...", "ja": "カラーマスクを追加（フィルター）...", "ko": "색상 마스크 추가 (필터)...", "tr": "RENK maskesi ekle (filtreler)...", "pl": "Dodaj maskę KOLORU (filtry)...", "nl": "KLEUR-masker toevoegen (filters)...", "sv": "Lägg till FÄRG-mask (filter)...", "hi": "रंग मास्क जोड़ें (फ़िल्टर)..."},
    "Eliminar mascara de color": {"en": "Delete color mask", "fr": "Supprimer le masque de couleur", "de": "Farbmaske löschen", "it": "Elimina maschera colore", "pt": "Excluir máscara de cor", "zh": "删除颜色蒙版", "ar": "حذف قناع اللون", "ru": "Удалить цветную маску", "ja": "カラーマスクを削除", "ko": "색상 마스크 삭제", "tr": "Renk maskesini sil", "pl": "Usuń maskę koloru", "nl": "Kleurmasker verwijderen", "sv": "Ta bort färgmask", "hi": "रंग मास्क हटाएँ"},
    "Desactivar mascara de color": {"en": "Disable color mask", "fr": "Désactiver le masque de couleur", "de": "Farbmaske deaktivieren", "it": "Disattiva maschera colore", "pt": "Desativar máscara de cor", "zh": "禁用颜色蒙版", "ar": "تعطيل قناع اللون", "ru": "Отключить цветную маску", "ja": "カラーマスクを無効化", "ko": "색상 마스크 비활성화", "tr": "Renk maskesini devre dışı bırak", "pl": "Wyłącz maskę koloru", "nl": "Kleurmasker uitschakelen", "sv": "Inaktivera färgmask", "hi": "रंग मास्क अक्षम करें"},

    # ============================================================
    # DE FORM SETTINGS
    # ============================================================
    "Pincel de Deformacion": {"en": "Deform Brush", "fr": "Pinceau de déformation", "de": "Verformungspinsel", "it": "Pennello di deformazione", "pt": "Pincel de deformação", "zh": "变形画笔", "ar": "فرشاة التشويه", "ru": "Кисть деформации", "ja": "変形ブラシ", "ko": "변형 브러시", "tr": "Şekil bozma fırçası", "pl": "Pędzel deformacji", "nl": "Vervormpenseel", "sv": "Deformeringspensel", "hi": "विकृति ब्रश"},
    "Modo:": {"en": "Mode:", "fr": "Mode :", "de": "Modus:", "it": "Modalità:", "pt": "Modo:", "zh": "模式：", "ar": "الوضع:", "ru": "Режим:", "ja": "モード：", "ko": "모드:", "tr": "Mod:", "pl": "Tryb:", "nl": "Modus:", "sv": "Läge:", "hi": "मोड:"},
    "Empujar": {"en": "Push", "fr": "Pousser", "de": "Schieben", "it": "Spingi", "pt": "Empurrar", "zh": "推动", "ar": "دفع", "ru": "Толкать", "ja": "押し出し", "ko": "밀기", "tr": "İt", "pl": "Pchnij", "nl": "Duwen", "sv": "Tryck", "hi": "धक्का"},
    "Girar horario": {"en": "Twirl CW", "fr": "Tourner horaire", "de": "Drehen im Uhrzeigersinn", "it": "Ruota orario", "pt": "Girar horário", "zh": "顺时针旋转", "ar": "تدوير باتجاه عقارب الساعة", "ru": "Вращать по часовой", "ja": "時計回りに回転", "ko": "시계 방향 회전", "tr": "Saat yönünde döndür", "pl": "Obróć w prawo", "nl": "Rechtsom draaien", "sv": "Rotera medurs", "hi": "घड़ी की दिशा में घुमाएँ"},
    "Girar antihorario": {"en": "Twirl CCW", "fr": "Tourner antihoraire", "de": "Drehen gegen den Uhrzeigersinn", "it": "Ruota antiorario", "pt": "Girar anti-horário", "zh": "逆时针旋转", "ar": "تدوير عكس عقارب الساعة", "ru": "Вращать против часовой", "ja": "反時計回りに回転", "ko": "반시계 방향 회전", "tr": "Saat yönünün tersine döndür", "pl": "Obróć w lewo", "nl": "Linksom draaien", "sv": "Rotera moturs", "hi": "घड़ी की विपरीत दिशा में घुमाएँ"},
    "Inflar": {"en": "Inflate", "fr": "Gonfler", "de": "Aufblähen", "it": "Gonfia", "pt": "Inflar", "zh": "膨胀", "ar": "تضخيم", "ru": "Раздуть", "ja": "膨らませる", "ko": "팽창", "tr": "Şişir", "pl": "Napompuj", "nl": "Opblazen", "sv": "Blås upp", "hi": "फुलाएँ"},
    "Desinflar": {"en": "Pinch", "fr": "Pincer", "de": "Zusammenziehen", "it": "Pizzica", "pt": "Pinçar", "zh": "收缩", "ar": "تضييق", "ru": "Сжать", "ja": "つまむ", "ko": "집기", "tr": "Sıkıştır", "pl": "Ściśnij", "nl": "Knijpen", "sv": "Nyp ihop", "hi": "पिंच"},
    "Ondas": {"en": "Waves", "fr": "Vagues", "de": "Wellen", "it": "Onde", "pt": "Ondas", "zh": "波浪", "ar": "أمواج", "ru": "Волны", "ja": "波", "ko": "파도", "tr": "Dalgalar", "pl": "Fale", "nl": "Golven", "sv": "Vågor", "hi": "लहरें"},
    "Radio: 40 px": {"en": "Radius: 40 px", "fr": "Rayon : 40 px", "de": "Radius: 40 px", "it": "Raggio: 40 px", "pt": "Raio: 40 px", "zh": "半径：40 像素", "ar": "نصف القطر: 40 بكسل", "ru": "Радиус: 40 px", "ja": "半径: 40 px", "ko": "반경: 40px", "tr": "Yarıçap: 40 px", "pl": "Promień: 40 px", "nl": "Straal: 40 px", "sv": "Radie: 40 px", "hi": "त्रिज्या: 40 px"},
    "Radio: %1 px": {"en": "Radius: %1 px", "fr": "Rayon : %1 px", "de": "Radius: %1 px", "it": "Raggio: %1 px", "pt": "Raio: %1 px", "zh": "半径：%1 像素", "ar": "نصف القطر: %1 بكسل", "ru": "Радиус: %1 px", "ja": "半径: %1 px", "ko": "반경: %1px", "tr": "Yarıçap: %1 px", "pl": "Promień: %1 px", "nl": "Straal: %1 px", "sv": "Radie: %1 px", "hi": "त्रिज्या: %1 px"},
    "Fuerza: 50%": {"en": "Strength: 50%", "fr": "Force : 50 %", "de": "Stärke: 50 %", "it": "Forza: 50%", "pt": "Força: 50%", "zh": "强度：50%", "ar": "القوة: 50%", "ru": "Сила: 50%", "ja": "強度: 50%", "ko": "강도: 50%", "tr": "Güç: %50", "pl": "Siła: 50%", "nl": "Sterkte: 50%", "sv": "Styrka: 50%", "hi": "शक्ति: 50%"},
    "Fuerza: %1%": {"en": "Strength: %1%", "fr": "Force : %1 %", "de": "Stärke: %1 %", "it": "Forza: %1%", "pt": "Força: %1%", "zh": "强度：%1%", "ar": "القوة: %1%", "ru": "Сила: %1%", "ja": "強度: %1%", "ko": "강도: %1%", "tr": "Güç: %1%", "pl": "Siła: %1%", "nl": "Sterkte: %1%", "sv": "Styrka: %1%", "hi": "शक्ति: %1%"},
    "Arrastra con clic izq.: deforma\nClic der.: invierte el efecto": {"en": "Drag with left click: deforms\nRight click: inverts effect", "fr": "Glisser avec clic gauche : déforme\nClic droit : inverse l'effet", "de": "Mit Linksklick ziehen: verformt\nRechtsklick: kehrt den Effekt um", "it": "Trascina con clic sinistro: deforma\nClic destro: inverte l'effetto", "pt": "Arraste com clique esquerdo: deforma\nClique direito: inverte o efeito", "zh": "左键拖动：变形\n右键：反转效果", "ar": "اسحب بالنقر الأيسر: يشوه\nالنقر الأيمن: يعكس التأثير", "ru": "Перетаскивание левой кнопкой: деформирует\nПравая кнопка: инвертирует эффект", "ja": "左クリックでドラッグ：変形\n右クリック：効果を反転", "ko": "왼쪽 클릭으로 드래그: 변형\n오른쪽 클릭: 효과 반전", "tr": "Sol tıkla sürükle: şekil bozar\nSağ tık: etkiyi ters çevirir", "pl": "Przeciągnij lewym przyciskiem: deformuje\nPrawy przycisk: odwraca efekt", "nl": "Sleep met linkermuisklik: vervormt\nRechtermuisklik: keert effect om", "sv": "Dra med vänsterklick: deformerar\nHögerklick: inverterar effekten", "hi": "बाएँ क्लिक से खींचें: विकृत करता है\nदाएँ क्लिक: प्रभाव उलटता है"},

    # ============================================================
    # PROPIEDADES DE FIGURA (ShapePropertiesDialog)
    # ============================================================
    "Propiedades de Figura": {"en": "Shape Properties", "fr": "Propriétés de la forme", "de": "Formeigenschaften", "it": "Proprietà forma", "pt": "Propriedades da forma", "zh": "形状属性", "ar": "خصائص الشكل", "ru": "Свойства фигуры", "ja": "図形のプロパティ", "ko": "도형 속성", "tr": "Şekil Özellikleri", "pl": "Właściwości kształtu", "nl": "Vormeigenschappen", "sv": "Formegenskaper", "hi": "आकृति गुण"},
    "Vista previa": {"en": "Preview", "fr": "Aperçu", "de": "Vorschau", "it": "Anteprima", "pt": "Pré-visualização", "zh": "预览", "ar": "معاينة", "ru": "Предпросмотр", "ja": "プレビュー", "ko": "미리보기", "tr": "Önizleme", "pl": "Podgląd", "nl": "Voorbeeld", "sv": "Förhandsgranskning", "hi": "पूर्वावलोकन"},
    "Color de relleno:": {"en": "Fill color:", "fr": "Couleur de remplissage :", "de": "Füllfarbe:", "it": "Colore di riempimento:", "pt": "Cor de preenchimento:", "zh": "填充颜色：", "ar": "لون التعبئة:", "ru": "Цвет заливки:", "ja": "塗りつぶしの色：", "ko": "채우기 색상:", "tr": "Dolgu rengi:", "pl": "Kolor wypełnienia:", "nl": "Opvulkleur:", "sv": "Fyllnadsfärg:", "hi": "भरण रंग:"},
    "Color de borde:": {"en": "Stroke color:", "fr": "Couleur du contour :", "de": "Randfarbe:", "it": "Colore del bordo:", "pt": "Cor da borda:", "zh": "描边颜色：", "ar": "لون الحدود:", "ru": "Цвет обводки:", "ja": "線の色：", "ko": "테두리 색상:", "tr": "Kenarlık rengi:", "pl": "Kolor obrysu:", "nl": "Randkleur:", "sv": "Kantfärg:", "hi": "स्ट्रोक रंग:"},
    "Grosor:": {"en": "Width:", "fr": "Épaisseur :", "de": "Dicke:", "it": "Spessore:", "pt": "Espessura:", "zh": "粗细：", "ar": "السماكة:", "ru": "Толщина:", "ja": "太さ：", "ko": "두께:", "tr": "Kalınlık:", "pl": "Grubość:", "nl": "Dikte:", "sv": "Tjocklek:", "hi": "मोटाई:"},
    "Figura HUECA (solo contorno, sin relleno)": {"en": "HOLLOW shape (outline only, no fill)", "fr": "Forme CREUSE (contour uniquement, sans remplissage)", "de": "HOHLE Form (nur Umriss, keine Füllung)", "it": "Forma VUOTA (solo contorno, senza riempimento)", "pt": "Forma VAZIA (apenas contorno, sem preenchimento)", "zh": "空心形状（仅轮廓，无填充）", "ar": "شكل مجوف (الحدود فقط، بدون تعبئة)", "ru": "ПОЛАЯ фигура (только контур, без заливки)", "ja": "中空図形（輪郭のみ、塗りなし）", "ko": "속이 빈 도형 (윤곽만, 채우기 없음)", "tr": "İÇİ BOŞ şekil (sadece dış çizgi, dolgu yok)", "pl": "PUSTY kształt (tylko kontur, bez wypełnienia)", "nl": "HOLLE vorm (alleen omtrek, geen vulling)", "sv": "IHÅLIG form (endast kontur, ingen fyllning)", "hi": "खोखली आकृति (केवल रूपरेखा, कोई भरण नहीं)"},
    "Usar como MARCO de imagen (estilo Canva)": {"en": "Use as IMAGE FRAME (Canva style)", "fr": "Utiliser comme CADRE d'image (style Canva)", "de": "Als BILDERRAHMEN verwenden (Canva-Stil)", "it": "Usa come CORNICE immagine (stile Canva)", "pt": "Usar como MOLDURA de imagem (estilo Canva)", "zh": "用作图像框架（Canva 风格）", "ar": "استخدام كإطار صورة (نمط Canva)", "ru": "Использовать как РАМКУ изображения (стиль Canva)", "ja": "画像フレームとして使用（Canva スタイル）", "ko": "이미지 프레임으로 사용 (Canva 스타일)", "tr": "RESİM ÇERÇEVESİ olarak kullan (Canva tarzı)", "pl": "Użyj jako RAMKA obrazu (styl Canva)", "nl": "Als AFBEELDINGSKADER gebruiken (Canva-stijl)", "sv": "Använd som BILDRAM (Canva-stil)", "hi": "छवि फ़्रेम के रूप में उपयोग करें (Canva शैली)"},
    "Cargar imagen en el marco...": {"en": "Load image into frame...", "fr": "Charger une image dans le cadre...", "de": "Bild in Rahmen laden...", "it": "Carica immagine nella cornice...", "pt": "Carregar imagem na moldura...", "zh": "将图像加载到框架中...", "ar": "تحميل صورة في الإطار...", "ru": "Загрузить изображение в рамку...", "ja": "フレームに画像を読み込む...", "ko": "프레임에 이미지 로드...", "tr": "Çerçeveye resim yükle...", "pl": "Załaduj obraz do ramki...", "nl": "Afbeelding in kader laden...", "sv": "Ladda bild i ram...", "hi": "फ़्रेम में छवि लोड करें..."},
    "Escala de la imagen:": {"en": "Image scale:", "fr": "Échelle de l'image :", "de": "Bildskalierung:", "it": "Scala immagine:", "pt": "Escala da imagem:", "zh": "图像比例：", "ar": "مقياس الصورة:", "ru": "Масштаб изображения:", "ja": "画像のスケール：", "ko": "이미지 크기 조정:", "tr": "Resim ölçeği:", "pl": "Skala obrazu:", "nl": "Afbeeldingsschaal:", "sv": "Bildskala:", "hi": "छवि स्केल:"},
    "Desplazar horizontal (X):": {"en": "Shift horizontally (X):", "fr": "Décaler horizontalement (X) :", "de": "Horizontal verschieben (X):", "it": "Sposta orizzontalmente (X):", "pt": "Deslocar horizontalmente (X):", "zh": "水平移动（X）：", "ar": "إزاحة أفقية (X):", "ru": "Сдвинуть по горизонтали (X):", "ja": "水平方向にシフト (X)：", "ko": "수평 이동 (X):", "tr": "Yatay kaydır (X):", "pl": "Przesuń w poziomie (X):", "nl": "Horizontaal verschuiven (X):", "sv": "Förskjut horisontellt (X):", "hi": "क्षैतिज शिफ्ट (X):"},
    "Desplazar vertical (Y):": {"en": "Shift vertically (Y):", "fr": "Décaler verticalement (Y) :", "de": "Vertikal verschieben (Y):", "it": "Sposta verticalmente (Y):", "pt": "Deslocar verticalmente (Y):", "zh": "垂直移动（Y）：", "ar": "إزاحة عمودية (Y):", "ru": "Сдвинуть по вертикали (Y):", "ja": "垂直方向にシフト (Y)：", "ko": "수직 이동 (Y):", "tr": "Dikey kaydır (Y):", "pl": "Przesuń w pionie (Y):", "nl": "Verticaal verschuiven (Y):", "sv": "Förskjut vertikalt (Y):", "hi": "लंबवत शिफ्ट (Y):"},
    "Sin imagen": {"en": "No image", "fr": "Sans image", "de": "Kein Bild", "it": "Nessuna immagine", "pt": "Sem imagem", "zh": "无图像", "ar": "لا توجد صورة", "ru": "Нет изображения", "ja": "画像なし", "ko": "이미지 없음", "tr": "Resim yok", "pl": "Brak obrazu", "nl": "Geen afbeelding", "sv": "Ingen bild", "hi": "कोई छवि नहीं"},
    "Imagen: %1 x %2 px": {"en": "Image: %1 x %2 px", "fr": "Image : %1 x %2 px", "de": "Bild: %1 x %2 px", "it": "Immagine: %1 x %2 px", "pt": "Imagem: %1 x %2 px", "zh": "图像：%1 x %2 像素", "ar": "الصورة: %1 × %2 بكسل", "ru": "Изображение: %1 x %2 пикс", "ja": "画像: %1 x %2 ピクセル", "ko": "이미지: %1 x %2 px", "tr": "Resim: %1 x %2 px", "pl": "Obraz: %1 x %2 px", "nl": "Afbeelding: %1 x %2 px", "sv": "Bild: %1 x %2 px", "hi": "छवि: %1 x %2 px"},
    "Figura hueca (solo contorno)": {"en": "Hollow shape (outline only)", "fr": "Forme creuse (contour uniquement)", "de": "Hohle Form (nur Umriss)", "it": "Forma vuota (solo contorno)", "pt": "Forma vazia (apenas contorno)", "zh": "空心形状（仅轮廓）", "ar": "شكل مجوف (الحدود فقط)", "ru": "Полая фигура (только контур)", "ja": "中空図形（輪郭のみ）", "ko": "속이 빈 도형 (윤곽만)", "tr": "İçi boş şekil (sadece dış çizgi)", "pl": "Pusty kształt (tylko kontur)", "nl": "Holle vorm (alleen omtrek)", "sv": "Ihålig form (endast kontur)", "hi": "खोखली आकृति (केवल रूपरेखा)"},
    "Figura rellena": {"en": "Filled shape", "fr": "Forme remplie", "de": "Gefüllte Form", "it": "Forma riempita", "pt": "Forma preenchida", "zh": "填充形状", "ar": "شكل معبأ", "ru": "Залитая фигура", "ja": "塗りつぶし図形", "ko": "채워진 도형", "tr": "Dolgulu şekil", "pl": "Wypełniony kształt", "nl": "Gevulde vorm", "sv": "Fylld form", "hi": "भरी हुई आकृति"},
    "Rotar": {"en": "Rotate", "fr": "Rotation", "de": "Drehen", "it": "Ruota", "pt": "Girar", "zh": "旋转", "ar": "تدوير", "ru": "Повернуть", "ja": "回転", "ko": "회전", "tr": "Döndür", "pl": "Obróć", "nl": "Draaien", "sv": "Rotera", "hi": "घुमाएँ"},
    "Editar": {"en": "Edit", "fr": "Éditer", "de": "Bearbeiten", "it": "Modifica", "pt": "Editar", "zh": "编辑", "ar": "تحرير", "ru": "Редактировать", "ja": "編集", "ko": "편집", "tr": "Düzenle", "pl": "Edytuj", "nl": "Bewerken", "sv": "Redigera", "hi": "संपादित करें"},
    "Integrar": {"en": "Integrate", "fr": "Intégrer", "de": "Integrieren", "it": "Integra", "pt": "Integrar", "zh": "整合", "ar": "دمج", "ru": "Интегрировать", "ja": "統合", "ko": "통합", "tr": "Entegre et", "pl": "Zintegruj", "nl": "Integreren", "sv": "Integrera", "hi": "एकीकृत करें"},
}

# ============================================================
# IDIOMAS
# ============================================================
IDIOMAS = {
    "es": "paintux_es.ts", "en": "paintux_en.ts", "fr": "paintux_fr.ts",
    "de": "paintux_de.ts", "it": "paintux_it.ts", "pt": "paintux_pt.ts",
    "zh": "paintux_zh.ts", "ar": "paintux_ar.ts", "ru": "paintux_ru.ts",
    "ja": "paintux_ja.ts", "ko": "paintux_ko.ts", "tr": "paintux_tr.ts",
    "pl": "paintux_pl.ts", "nl": "paintux_nl.ts", "sv": "paintux_sv.ts",
    "hi": "paintux_hi.ts",
}


# ============================================================
# UTILIDADES
# ============================================================
def find_tool(name):
    candidates = [
        f"/usr/lib/qt6/bin/{name}",
        f"/usr/lib/qt6/bin/{name}-qt6",
        f"/usr/lib/qt6/bin/{name}6",
        f"/usr/lib/x86_64-linux-gnu/qt6/bin/{name}",
        f"/usr/bin/{name}",
        f"/usr/bin/{name}-qt6",
        f"/usr/bin/{name}6",
        f"/usr/local/bin/{name}",
        f"/opt/qt6/bin/{name}",
    ]
    for path in candidates:
        if os.path.exists(path) and os.access(path, os.X_OK):
            return path
    return shutil.which(name)


def find_source_files(src_dir):
    files = []
    src_path = Path(src_dir)
    if not src_path.exists():
        return files
    for ext in ("*.cpp", "*.h", "*.hpp", "*.cc", "*.cxx"):
        files.extend(str(p) for p in src_path.rglob(ext))
    return sorted(files)


# ============================================================
# LUPDATE
# ============================================================
def ejecutar_lupdate(src_dir="src"):
    print("\n [1/3] Ejecutando lupdate sobre src/...")
    lupdate = find_tool("lupdate")
    if not lupdate:
        print("  No se encontró lupdate. Instala: sudo apt install qt6-l10n-tools")
        return False
    print(f"   ✓ lupdate: {lupdate}")
    source_files = find_source_files(src_dir)
    if not source_files:
        print(f"  No se encontraron archivos fuente en {src_dir}/")
        return False
    print(f"   ✓ {len(source_files)} archivos fuente encontrados")
    ts_files = list(IDIOMAS.values())
    cmd = [lupdate, "-no-obsolete"] + source_files + ["-ts"] + ts_files
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
        if result.returncode == 0:
            print(f"   ✓ lupdate completado")
            for line in result.stdout.strip().split("\n")[-5:]:
                print(f"     {line.strip()}")
            return True
        else:
            print(f"  lupdate código {result.returncode}")
            if result.stderr.strip():
                print(f"   stderr: {result.stderr.strip()}")
            return False
    except subprocess.TimeoutExpired:
        print(" lupdate timeout")
        return False
    except Exception as e:
        print(f" Error: {e}")
        return False


# ============================================================
# TRADUCIR
# ============================================================
def traducir_archivo(filename, lang_code):
    if not os.path.exists(filename):
        return False, 0, 0, []
    try:
        tree = ET.parse(filename)
        root = tree.getroot()
        count = 0
        total = 0
        missing = []
        for context in root.findall('context'):
            for message in context.findall('message'):
                source = message.find('source')
                translation = message.find('translation')
                if source is None or translation is None:
                    continue
                total += 1
                texto = source.text or ""
                if texto in TRADUCCIONES and lang_code in TRADUCCIONES[texto]:
                    translation.text = TRADUCCIONES[texto][lang_code]
                    if 'type' in translation.attrib:
                        del translation.attrib['type']
                    count += 1
                else:
                    missing.append(texto)
        indent_xml(root)
        tree.write(filename, encoding='utf-8', xml_declaration=True)
        return True, count, total, missing
    except Exception as e:
        print(f"✗ Error en {filename}: {e}")
        return False, 0, 0, []


def indent_xml(elem, level=0):
    i = "\n" + level * "    "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "    "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for child in elem:
            indent_xml(child, level + 1)
        if not child.tail or not child.tail.strip():
            child.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i


# ============================================================
# LRELEASE
# ============================================================
def compilar_qm():
    print("\n [3/3] Compilando .ts a .qm con lrelease...")
    lrelease = find_tool("lrelease")
    if not lrelease:
        print("  No se encontró lrelease. Instala: sudo apt install qt6-l10n-tools")
        return False
    print(f"   ✓ lrelease: {lrelease}")
    ts_files = sorted([f for f in os.listdir(".") if f.startswith("paintux_") and f.endswith(".ts")])
    if not ts_files:
        print("  No hay archivos .ts")
        return False
    ok = 0
    fail = 0
    for ts in ts_files:
        try:
            result = subprocess.run([lrelease, ts], capture_output=True, text=True, timeout=60)
            qm = ts.replace(".ts", ".qm")
            if result.returncode == 0 and os.path.exists(qm):
                print(f"   ✓ {ts} → {qm}")
                ok += 1
            else:
                print(f"   ✗ {ts}: {result.stderr.strip()}")
                fail += 1
        except Exception as e:
            print(f"   ✗ {ts}: {e}")
            fail += 1
    print(f"\n   Total: {ok} OK, {fail} fallidos")
    return fail == 0


# ============================================================
# MAIN
# ============================================================
if __name__ == "__main__":
    print("=" * 70)
    print(" Paint-UX - Sistema de Traducción Multiidioma (16 idiomas)")
    print("=" * 70)
    print(f" Diccionario: {len(TRADUCCIONES)} textos únicos")
    print("=" * 70)

    src_dir = "../src"
    if not os.path.exists(src_dir):
        src_dir = "src"
    if not os.path.exists(src_dir):
        src_dir = "."

    print(f" Fuentes en: {os.path.abspath(src_dir)}")

    ejecutar_lupdate(src_dir)

    print("\n [2/3] Aplicando traducciones del diccionario...")
    print("-" * 70)

    total_ok = 0
    total_missing = set()
    for codigo, archivo in sorted(IDIOMAS.items()):
        ok, count, total, missing = traducir_archivo(archivo, codigo)
        if ok:
            pct = (count * 100 // total) if total > 0 else 0
            print(f"   ✓ {archivo}: {count}/{total} ({pct}%) [{codigo}]")
            total_ok += count
            total_missing.update(missing)
        else:
            print(f"   ✗ {archivo}: no se pudo procesar")

    print("-" * 70)
    print(f"   Total traducidos: {total_ok}")

    if total_missing:
        print(f"\n     Faltan {len(total_missing)} textos en el diccionario")
        print(f"      (Añádelos al diccionario de traducir.py)")

    compilar_qm()

    print("\n" + "=" * 70)
    print(" TRADUCCIÓN COMPLETA")
    print("=" * 70)
    print("\nAhora compila el proyecto:")
    print("   cd ../build && cmake .. && make -j$(nproc)")
    print("=" * 70)