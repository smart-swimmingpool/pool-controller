#!/usr/bin/env python3
"""
Generate KiCad 9.0 schematic files for the Pool Controller project.

Two variants:
  1) Standard ESP32 Dev Board (external relay module, DS18B20 on individual GPIOs)
  2) NORVI AE01-R industrial controller (built-in relays, shared OneWire bus)

Outputs .kicad_sch + .kicad_pro files in docs/kicad/<variant>/
"""

import os
import uuid
import subprocess

from sexpdata import dumps, Symbol

# ─── helpers ──────────────────────────────────────────────────────────────

def uu():
    return str(uuid.uuid4())

def S(name):
    """Create an unquoted S-expression Symbol (keyword/enum)."""
    return Symbol(name)

def Q(s):
    """Create a quoted string value — just use a Python string."""
    return s  # Python strings serialize to "quoted" in sexpdata

def At(x, y, rot=0):
    """Position: (at x y rot)"""
    return [S("at"), round(x, 4), round(y, 4), rot]

def Pt(x, y):
    """Point: (xy x y)"""
    return [S("xy"), round(x, 4), round(y, 4)]

# ─── Symbol builders ─────────────────────────────────────────────────────

def font_props(size=1.27):
    return [S("font"), [S("size"), size, size]]

def make_rect_symbol(name, description, ref_designator, width, height, pins):
    """
    Create an embedded symbol definition.
    pins: list of (pin_name, pin_number, x, y, orientation, electrical_type)
    """
    body = [
        S("symbol"), Q(name),
        [S("pin_names"), [S("offset"), 1.016]],
        [S("exclude_from_sim"), S("no")],
        [S("in_bom"), S("yes")],
        [S("on_board"), S("yes")],
        [S("property"), Q("Reference"), Q(ref_designator),
         At(0, -(height/2 + 5)),
         [S("effects"), font_props(1.778)]],
        [S("property"), Q("Value"), Q(name),
         At(0, height/2 + 5),
         [S("effects"), font_props(1.778)]],
        [S("property"), Q("Footprint"), Q(""),
         At(0, 0), [S("effects"), font_props(1.016), [S("hide"), S("yes")]]],
        [S("property"), Q("Datasheet"), Q(""),
         At(0, 0), [S("effects"), font_props(1.016), [S("hide"), S("yes")]]],
        [S("property"), Q("Description"), Q(description),
         At(0, 0), [S("effects"), font_props(1.27), [S("hide"), S("yes")]]],
    ]

    hw, hh = width/2, height/2
    body.append([
        S("symbol"), Q(f"{name}_0_1"),
        [S("rectangle"),
         [S("start"), -hw, -hh],
         [S("end"), hw, hh],
         [S("stroke"), [S("width"), 0], [S("type"), S("default")]],
         [S("fill"), [S("type"), S("none")]]]
    ])

    pins_part = [S("symbol"), Q(f"{name}_1_1")]
    for pin_name, pin_num, px, py, orient, etype in pins:
        pins_part.append([
            S("pin"), S(etype), S("line"),
            At(px, py, orient),
            [S("length"), 5.08],
            [S("name"), Q(pin_name), [S("effects"), font_props(1.27)]],
            [S("number"), Q(str(pin_num)), [S("effects"), font_props(1.27)]]
        ])
    body.append(pins_part)
    return body


# ─── Pin definitions ─────────────────────────────────────────────────────

ESP32_DEV_PINS = [
    (Q("3V3"),         "1", -20,  70, 180, "power_in"),
    (Q("GND"),         "2", -20,  60, 180, "power_in"),
    (Q("GPIO32"),      "3", -20,  30, 180, "bidirectional"),
    (Q("GPIO33"),      "4", -20,  20, 180, "bidirectional"),
    (Q("GPIO25"),      "5", -20, -20, 180, "output"),
    (Q("GPIO26"),      "6", -20, -30, 180, "output"),
    (Q("GPIO2_LED"),   "7", -20, -50, 180, "output"),
    (Q("VIN_5V"),      "8", -20, -70, 180, "power_in"),
]

DS18B20_PINS = [
    (Q("VDD"),  "1",  20,  15, 0, "power_in"),
    (Q("DATA"), "2",  20,   0, 0, "bidirectional"),
    (Q("GND"),  "3",  20, -15, 0, "power_in"),
]

RELAY2CH_PINS = [
    (Q("IN1"),  "1", -20,  20, 180, "input"),
    (Q("IN2"),  "2", -20,  10, 180, "input"),
    (Q("GND"),  "3", -20,  -5, 180, "power_in"),
    (Q("VCC"),  "4", -20, -15, 180, "power_in"),
    (Q("COM1"), "5",  20,  20,   0, "passive"),
    (Q("NO1"),  "6",  20,  12,   0, "passive"),
    (Q("COM2"), "7",  20,  -5,   0, "passive"),
    (Q("NO2"),  "8",  20, -13,   0, "passive"),
]

RES_PINS = [
    (Q("~"), "1", -10, 0, 180, "passive"),
    (Q("~"), "2",  10, 0,   0, "passive"),
]

NORVI_PINS = [
    (Q("24V_IN"),    "1", -20,  70, 180, "power_in"),
    (Q("GND"),       "2", -20,  60, 180, "power_in"),
    (Q("GPIO25_DAT"),"3", -20,  35, 180, "bidirectional"),
    (Q("GPIO14_R0"), "4", -20,  15, 180, "output"),
    (Q("GPIO12_R1"), "5", -20,   5, 180, "output"),
    (Q("GPIO27_LED"),"6", -20, -10, 180, "output"),
    (Q("GPIO16_SDA"),"7", -20, -30, 180, "bidirectional"),
    (Q("GPIO17_SCL"),"8", -20, -40, 180, "bidirectional"),
    (Q("GPIO32_BTN"),"9", -20, -60, 180, "input"),
    (Q("GPIO5_ALT"), "10", -20, -75, 180, "bidirectional"),
]

LED_PINS = [
    # Horizontal orientation: anode (A) exits left at x=−10, cathode (C) exits right at x=+10
    (Q("A"), "1", -10, 0, 180, "passive"),
    (Q("C"), "2",  10, 0,   0, "passive"),
]

# ─── Schematic builder ───────────────────────────────────────────────────

class Schematic:
    def __init__(self, title, rev="1.0", company="Smart Swimmingpool"):
        self.title = title
        self.rev = rev
        self.company = company
        self.doc_uuid = uu()
        self.embedded_symbols = []
        self.instances = []
        self.wires = []
        self.labels = []
        self.global_labels = []
        self.junctions = []
        self.texts = []

    def add_part(self, lib_id, value, ref, x, y, rot=0):
        uid = uu()
        self.instances.append((uid, lib_id, value, ref, x, y, rot))
        return uid

    def wire(self, x1, y1, x2, y2):
        self.wires.append(((x1, y1), (x2, y2)))

    def label(self, name, x, y, orient=0):
        self.labels.append((name, x, y, orient))

    def power(self, name, x, y, etype="power_in"):
        self.global_labels.append((name, x, y, 0, etype))

    def junction(self, x, y):
        self.junctions.append((x, y))

    def text(self, t, x, y, size=1.27):
        self.texts.append((t, x, y, size))

    def add_symbols(self, *symbols_list):
        self.embedded_symbols.extend(symbols_list)

    def gnd(self, x, y):
        self.power("GND", x, y)

    def _sexp(self):
        """Build the complete S-expression tree."""
        tree = [
            S("kicad_sch"),
            [S("version"), 20250114],
            [S("generator"), Q("eeschema")],
            [S("generator_version"), Q("9.0")],
            [S("uuid"), Q(self.doc_uuid)],
            [S("paper"), Q("A4")],
            [S("title_block"),
             [S("title"), Q(self.title)],
             [S("date"), Q("2026-06-16")],
             [S("rev"), Q(self.rev)],
             [S("company"), Q(self.company)]],
        ]

        # lib_symbols
        if self.embedded_symbols:
            lib = [S("lib_symbols")]
            for sym in self.embedded_symbols:
                lib.append(sym)
            tree.append(lib)

        # Symbol instances
        for inst_id, lib_id, value, ref, x, y, rot in self.instances:
            inst = [
                S("symbol"),
                [S("lib_id"), Q(lib_id)],
                At(x, y, rot),
                [S("uuid"), Q(inst_id)],
                [S("property"), Q("Reference"), Q(ref),
                 At(x, y + 5),
                 [S("effects"), font_props(1.27)]],
                [S("property"), Q("Value"), Q(value),
                 At(x, y - 5),
                 [S("effects"), font_props(1.27)]],
                [S("instances"),
                 [S("project"), Q(self.title),
                  [S("path"), Q(f"/{self.doc_uuid}"),
                   [S("reference"), Q(ref)],
                   [S("unit"), 1]]]]
            ]
            tree.append(inst)

        # Wires
        for (x1, y1), (x2, y2) in self.wires:
            tree.append([
                S("wire"),
                [S("pts"), Pt(x1, y1), Pt(x2, y2)],
                [S("stroke"), [S("width"), 0], [S("type"), S("default")]],
                [S("uuid"), Q(uu())]
            ])

        # Junctions
        for x, y in self.junctions:
            tree.append([
                S("junction"),
                [S("at"), round(x, 4), round(y, 4)],
                [S("diameter"), 0.8128],
                [S("color"), 0, 0, 0, 0],
                [S("uuid"), Q(uu())]
            ])

        # Labels
        for name, x, y, orient in self.labels:
            tree.append([
                S("label"), Q(name),
                At(x, y, orient),
                [S("effects"), font_props(1.27)],
                [S("uuid"), Q(uu())]
            ])

        # Global labels (power symbols — no pin sub-element in KiCad 9.0)
        for name, x, y, orient, etype in self.global_labels:
            tree.append([
                S("global_label"), Q(name),
                At(x, y, orient),
                [S("effects"), font_props(1.27)],
                [S("uuid"), Q(uu())]
            ])

        # Text notes
        for t, x, y, size in self.texts:
            tree.append([
                S("text"), Q(t),
                At(x, y),
                [S("effects"), font_props(size)],
                [S("uuid"), Q(uu())]
            ])

        # sheet_instances
        tree.append([
            S("sheet_instances"),
            [S("path"), Q("/"),
             [S("page"), Q("1")]]
        ])

        # embedded_fonts
        tree.append([S("embedded_fonts"), S("no")])

        return tree

    def save(self, path):
        tree = self._sexp()
        sexp_str = dumps(tree)

        # sexpdata produces compact S-expressions without pretty-print.
        # KiCad expects keywords/atoms on the same line as the opening paren.
        # We need to pretty-print manually with the KiCad convention:
        #   (keyword
        #       (sub1 ...)
        #       (sub2 ...)
        #   )
        # Only indent lists, keep atoms on the same line as their parent.
        result = self._pretty_sexp(tree, 0)
        content = '\n'.join(result)

        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as f:
            f.write(content + '\n')
        print(f"  ✓ {path}")

    def _pretty_sexp(self, item, depth):
        """Convert item to KiCad-compatible S-expression lines."""
        indent = '\t' * depth
        if isinstance(item, list):
            if not item:
                return [f"{indent}()"]

            # Gather leading simple (non-list) items onto the opening line
            # e.g. (symbol "NAME" ...) or (size 1.27 1.27)
            opening_items = []
            remaining = []
            for i, elem in enumerate(item):
                if i == 0 or not isinstance(elem, list):
                    opening_items.append(elem)
                else:
                    remaining = item[i:]
                    break
            else:
                # All items are simple → single-line (space-separated)
                parts = [f"{indent}("]
                for i, e in enumerate(opening_items):
                    if i > 0:
                        parts.append(' ')
                    parts.append(self._sexp_str(e))
                parts.append(')')
                return [''.join(parts)]

            # Opening line: indent + ( + first + space-separated simple followers
            open_parts = [f"{indent}("]
            for i, e in enumerate(opening_items):
                if i > 0:
                    open_parts.append(' ')
                open_parts.append(self._sexp_str(e))
            first_line = ''.join(open_parts)

            lines = [first_line]

            # Remaining complex items each get their own indented line
            for r in remaining:
                child_lines = self._pretty_sexp(r, depth + 1)
                lines.extend(child_lines)

            lines.append(f"{indent})")
            return lines

        else:
            return [f"{indent}{self._sexp_str(item)}"]

    def _sexp_str(self, item):
        """Convert a single item to its string representation."""
        if isinstance(item, Symbol):
            return item._val if hasattr(item, '_val') else str(item)
        elif isinstance(item, str):
            # Escape quotes in strings
            escaped = item.replace('\\', '\\\\').replace('"', '\\"')
            return f'"{escaped}"'
        elif isinstance(item, bool):
            return 'yes' if item else 'no'
        else:
            return str(item)


def create_project_file(sch_path, title, company="Smart Swimmingpool", rev="1.0"):
    pro_path = sch_path.replace('.kicad_sch', '.kicad_pro')
    pro_uuid = uu()
    basename = os.path.basename(pro_path)

    template = f'''{{
  "board": {{
    "design_settings": {{ "defaults": {{}}, "rules": {{}} }},
    "layer_presets": [],
    "meta": {{ "version": 1 }},
    "stackup": {{}}
  }},
  "boards": [],
  "cvpcb": {{ "equivalence_files": [] }},
  "erc": {{
    "erc_exclusions": [],
    "meta": {{ "version": 1 }},
    "pin_map": []
  }},
  "libraries": {{
    "pinned_footprint_libs": [],
    "pinned_symbol_libs": []
  }},
  "meta": {{ "filename": "{basename}", "version": 1 }},
  "net_settings": {{
    "classes": [],
    "meta": {{ "version": 1 }},
    "net_colors": null
  }},
  "pcbnew": {{
    "last_paths": {{}},
    "page_layout_descr_file": ""
  }},
  "project": {{ "name": "{title}", "uuid": "{pro_uuid}" }},
  "schematic": {{
    "drawing": {{
      "default_line_width": 0,
      "default_text_size": 1.27,
      "field_names": [],
      "intersheets_ref_owners": [],
      "intersheets_ref_prefix": "",
      "page_layout_descr_file": "",
      "plot_directory": "",
      "sheet": {{ "page_number": 1, "page_count": 1 }}
    }},
    "meta": {{ "version": 1 }},
    "net_bus_group_colors": {{}},
    "net_colors": null,
    "page_layout_descr_file": "",
    "plot_params": {{}},
    "title_block": {{
      "company": "{company}",
      "date": "2026-06-16",
      "rev": "{rev}",
      "title": "{title}"
    }}
  }}
}}
'''
    with open(pro_path, 'w') as f:
        f.write(template)
    print(f"  ✓ {pro_path}")


def export_pdf(sch_path, output_path):
    try:
        r = subprocess.run(
            ["kicad-cli", "sch", "export", "pdf", sch_path, "--output", output_path],
            capture_output=True, text=True, timeout=30
        )
        if os.path.exists(output_path) and os.path.getsize(output_path) > 0:
            print(f"  ✓ PDF: {output_path}")
        else:
            print(f"  ⚠ PDF failed: {r.stderr.strip()}")
    except Exception as e:
        print(f"  ⚠ PDF export: {e}")

def export_svg(sch_path, output_dir):
    try:
        os.makedirs(output_dir, exist_ok=True)
        r = subprocess.run(
            ["kicad-cli", "sch", "export", "svg", sch_path, "--output", output_dir],
            capture_output=True, text=True, timeout=30
        )
        if os.listdir(output_dir):
            print(f"  ✓ SVG: {output_dir}")
        else:
            print(f"  ⚠ SVG failed: {r.stderr.strip()}")
    except Exception as e:
        print(f"  ⚠ SVG export: {e}")


# ═══════════════════════════════════════════════════════════════════════
#  VARIANT 1: Standard ESP32 Dev Board
# ═══════════════════════════════════════════════════════════════════════

def build_variant1(out_dir):
    print("\n=== Variant 1: Standard ESP32 Dev Board ===")

    sch = Schematic("Pool Controller — ESP32 Dev Board Schematic")

    sch.add_symbols(
        make_rect_symbol("ESP32_DEV_BOARD", "ESP32 Development Board", "U",
                         40, 160, ESP32_DEV_PINS),
        make_rect_symbol("DS18B20", "Digital Temperature Sensor", "U",
                         20, 30, DS18B20_PINS),
        make_rect_symbol("RELAY_2CH", "2-Channel Relay Module (5V, Optocoupler)", "U",
                         40, 40, RELAY2CH_PINS),
        make_rect_symbol("R_4K7", "Resistor 4.7kΩ", "R",
                         20, 10, RES_PINS),
    )

    # ESP32 Dev Board at center
    sch.add_part("ESP32_DEV_BOARD", "ESP32 Dev Board", "U1", 100, 75)

    # DS18B20 sensors on left
    sch.add_part("DS18B20", "DS18B20 Solar", "U2", 20, 110)
    sch.add_part("DS18B20", "DS18B20 Pool", "U3", 20, 40)

    # Pull-up resistors — vertical (rot=90) so Pin1 is on the data line and Pin2 goes to +3.3V.
    # For rot=90 with RES_PINS at py=±10: y_abs = y_inst ∓ 10 → Pin1=(xi, yi+10) / Pin2=(xi, yi−10).
    # R1 at (60, 100) rot=90: Pin1=(60,110) data side, Pin2=(60,90) VDD side
    sch.add_part("R_4K7", "4.7kΩ", "R1", 60, 100, 90)
    # R2 at (60, 30) rot=90: Pin1=(60,40) data side, Pin2=(60,20) VDD side
    sch.add_part("R_4K7", "4.7kΩ", "R2", 60, 30, 90)

    # Relay module on right
    sch.add_part("RELAY_2CH", "2-Ch Relay Module", "K1", 160, 60)

    # ═══ WIRING ═══
    # Absolute pin coordinates (y_abs = y_inst − py_pin; x_abs = x_inst + px_pin):
    #   U1 (100,75): 3V3=(80,5) GND=(80,15) GPIO32=(80,45) GPIO33=(80,55)
    #                GPIO25=(80,95) GPIO26=(80,105) VIN_5V=(80,145)
    #   U2 (20,110): VDD=(40,95) DATA=(40,110) GND=(40,125)
    #   U3 (20,40):  VDD=(40,25) DATA=(40,40)  GND=(40,55)
    #   R1 (60,100) rot=90: Pin1=(60,110) Pin2=(60,90)
    #   R2 (60,30)  rot=90: Pin1=(60,40)  Pin2=(60,20)
    #   K1 (160,60): IN1=(140,40) IN2=(140,50) GND=(140,65) VCC=(140,75)

    # ── Power symbols on each supply pin ──
    sch.power("+3.3V", 80, 5)       # U1 3V3 pin
    sch.gnd(80, 15)                 # U1 GND pin
    sch.power("+5V", 80, 145)       # U1 VIN_5V for relay module

    sch.power("+3.3V", 40, 95)      # U2 Solar VDD
    sch.gnd(40, 125)                # U2 Solar GND
    sch.power("+3.3V", 40, 25)      # U3 Pool VDD
    sch.gnd(40, 55)                 # U3 Pool GND

    sch.power("+3.3V", 60, 90)      # R1 Pin2 (pull-up VDD)
    sch.power("+3.3V", 60, 20)      # R2 Pin2 (pull-up VDD)

    sch.gnd(140, 65)                # K1 GND
    sch.power("+5V", 140, 75)       # K1 VCC

    # ── Solar data: U2 DATA (40,110) → R1 Pin1 (60,110) → net label "GPIO32" ──
    sch.wire(40, 110, 65, 110)
    sch.junction(60, 110)           # junction at R1 Pin1 on the data wire
    sch.label("GPIO32", 65, 110, 0)
    sch.label("GPIO32", 80, 45, 0)  # same net label at ESP32 GPIO32 pin

    # ── Pool data: U3 DATA (40,40) → R2 Pin1 (60,40) → net label "GPIO33" ──
    sch.wire(40, 40, 65, 40)
    sch.junction(60, 40)            # junction at R2 Pin1 on the data wire
    sch.label("GPIO33", 65, 40, 0)
    sch.label("GPIO33", 80, 55, 0)  # same net label at ESP32 GPIO33 pin

    # ── Relay control via matching net labels ──
    sch.label("GPIO25", 80, 95, 0)      # at ESP32 GPIO25 pin
    sch.label("GPIO25", 140, 40, 180)   # at K1 IN1 pin
    sch.label("GPIO26", 80, 105, 0)     # at ESP32 GPIO26 pin
    sch.label("GPIO26", 140, 50, 180)   # at K1 IN2 pin

    # ═══ LABELS ═══
    sch.text("DS18B20 Solar", 20, 125, 1.5)
    sch.text("DS18B20 Pool", 20, 55, 1.5)
    sch.text("4.7kΩ pull-up to 3V3", 62, 105, 1.0)
    sch.text("4.7kΩ pull-up to 3V3", 62, 35, 1.0)
    sch.text("ESP32 Dev Board", 100, 100, 1.5)
    sch.text("2-Ch Relay Module", 160, 85, 1.5)

    sch_path = os.path.join(out_dir, "esp32-dev-board.kicad_sch")
    sch.save(sch_path)
    create_project_file(sch_path, "Pool Controller — ESP32 Dev Board")

    pdf_path = os.path.join(out_dir, "esp32-dev-board-schematic.pdf")
    svg_dir = os.path.join(out_dir, "svg")
    export_pdf(sch_path, pdf_path)
    export_svg(sch_path, svg_dir)


# ═══════════════════════════════════════════════════════════════════════
#  VARIANT 2: NORVI AE01-R
# ═══════════════════════════════════════════════════════════════════════

def build_variant2(out_dir):
    print("\n=== Variant 2: NORVI AE01-R ===")

    sch = Schematic("Pool Controller — NORVI AE01-R Schematic")

    sch.add_symbols(
        make_rect_symbol("NORVI_AE01R", "NORVI IIOT-AE01-R Industrial Controller", "U",
                         40, 170, NORVI_PINS),
        make_rect_symbol("DS18B20", "Digital Temperature Sensor", "U",
                         20, 30, DS18B20_PINS),
        make_rect_symbol("R_4K7", "Resistor 4.7kΩ", "R",
                         20, 10, RES_PINS),
        make_rect_symbol("R_330", "Resistor 330Ω", "R",
                         20, 10, RES_PINS),
        make_rect_symbol("LED", "Light Emitting Diode", "D",
                         20, 15, LED_PINS),
    )

    sch.add_part("NORVI_AE01R", "NORVI AE01-R", "U1", 100, 75)
    sch.add_part("DS18B20", "DS18B20 Solar", "U2", 15, 105)
    sch.add_part("DS18B20", "DS18B20 Pool", "U3", 15, 80)
    # R1 horizontal (rot=0) at (65, 90): x_abs = x_inst ∓ 10 → Pin1=(55,90) data-bus, Pin2=(75,90) +3.3V
    sch.add_part("R_4K7", "4.7kΩ", "R1", 65, 90)
    sch.add_part("R_330", "330Ω", "R2", 130, 30)
    sch.add_part("LED", "Status LED", "D1", 150, 30)

    # ═══ WIRING ═══
    # Absolute pin coordinates (y_abs = y_inst − py_pin):
    #   U1 (100,75): 24V_IN=(80,5) GND=(80,15) GPIO25_DAT=(80,40)
    #                GPIO14_R0=(80,60) GPIO12_R1=(80,70) GPIO27_LED=(80,85)
    #   U2 (15,105): VDD=(35,90)  DATA=(35,105) GND=(35,120)
    #   U3 (15,80):  VDD=(35,65)  DATA=(35,80)  GND=(35,95)
    #   R1 (65,90):  Pin1=(55,90) Pin2=(75,90)
    #   R2 (130,30): Pin1=(120,30) Pin2=(140,30)
    #   D1 (150,30): A=(140,30) C=(160,30)

    # ── Power symbols ──
    sch.power("+24V", 80, 5)        # U1 24V_IN
    sch.gnd(80, 15)                 # U1 GND
    sch.power("+3.3V", 35, 90)      # U2 Solar VDD
    sch.gnd(35, 120)                # U2 Solar GND
    sch.power("+3.3V", 35, 65)      # U3 Pool VDD
    sch.gnd(35, 95)                 # U3 Pool GND
    sch.power("+3.3V", 75, 90)      # R1 Pin2 (pull-up VDD)
    sch.gnd(160, 30)                # D1 Cathode → GND

    # ── Shared 1-Wire data bus at x=50 ──
    # Vertical bus connecting NORVI GPIO25 (80,40) ↔ Pool DATA (35,80) ↔ Solar DATA (35,105)
    sch.wire(50, 40, 50, 105)       # vertical data bus
    sch.wire(50, 40, 80, 40)        # bus → NORVI GPIO25_DAT
    sch.wire(35, 105, 50, 105)      # U2 Solar DATA → bus
    sch.wire(35, 80, 50, 80)        # U3 Pool DATA → bus
    sch.junction(50, 80)
    sch.junction(50, 105)

    # R1 pull-up: short wire from bus at (50,90) to R1 Pin1 (55,90)
    sch.wire(50, 90, 55, 90)
    sch.junction(50, 90)            # junction on vertical data bus

    # ── Relay net labels (built-in relays, no external component) ──
    sch.label("GPIO14_R0", 80, 60, 0)    # Pool Pump relay output
    sch.label("GPIO12_R1", 80, 70, 0)    # Solar Pump relay output

    # ── Status LED circuit ──
    # GPIO27_LED (80,85) → R2 Pin1 (120,30) via net label → R2 Pin2 (140,30) → D1 A (140,30) → D1 C (160,30) → GND
    sch.label("GPIO27_LED", 80, 85, 0)      # at NORVI GPIO27 pin
    sch.label("GPIO27_LED", 120, 30, 180)   # at R2 Pin1 (label text goes left)

    # ═══ LABELS ═══
    sch.text("DS18B20 Solar", 15, 120, 1.5)
    sch.text("DS18B20 Pool", 15, 95, 1.5)
    sch.text("Shared 4.7kΩ pull-up", 55, 85, 1.0)
    sch.text("NORVI AE01-R", 100, 100, 1.5)
    sch.text("Status LED", 145, 45, 1.0)
    sch.text("330Ω", 130, 20, 1.0)
    sch.text("Built-in Relays:", 85, 65, 1.27)
    sch.text("R0 (GPIO14) → Pool Pump", 85, 60, 1.0)
    sch.text("R1 (GPIO12) → Solar Pump", 85, 70, 1.0)

    sch_path = os.path.join(out_dir, "norvi-ae01-r.kicad_sch")
    sch.save(sch_path)
    create_project_file(sch_path, "Pool Controller — NORVI AE01-R")

    pdf_path = os.path.join(out_dir, "norvi-ae01-r-schematic.pdf")
    svg_dir = os.path.join(out_dir, "svg")
    export_pdf(sch_path, pdf_path)
    export_svg(sch_path, svg_dir)


# ═══════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    base_dir = os.path.join(os.path.dirname(__file__), "..", "docs", "kicad")

    build_variant1(os.path.join(base_dir, "esp32-dev-board"))
    build_variant2(os.path.join(base_dir, "norvi-ae01-r"))

    print("\n✅ Done!")
