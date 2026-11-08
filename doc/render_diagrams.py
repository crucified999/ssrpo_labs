#!/usr/bin/env python3
"""Rebuild the report's static UML figures using only the Python standard library."""
from html import escape
from pathlib import Path

DESTINATION = Path(__file__).resolve().parent
INK = "#203147"
MUTED = "#53657b"


class Svg:
    def __init__(self, width, height, title):
        self.parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
                      f'viewBox="0 0 {width} {height}" role="img" aria-label="{escape(title)}">',
                      f'<title>{escape(title)}</title>', '''<defs>
<marker id="arrow" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto"><path d="M1,1 L9,5 L1,9" fill="none" stroke="#203147" stroke-width="1.4"/></marker>
<marker id="triangle" markerWidth="14" markerHeight="14" refX="13" refY="7" orient="auto"><path d="M1,1 L13,7 L1,13 Z" fill="white" stroke="#203147"/></marker>
<marker id="diamond" markerWidth="14" markerHeight="10" refX="1" refY="5" orient="auto"><path d="M1,5 L7,1 L13,5 L7,9 Z" fill="#203147"/></marker>
<marker id="aggregate" markerWidth="14" markerHeight="10" refX="1" refY="5" orient="auto"><path d="M1,5 L7,1 L13,5 L7,9 Z" fill="white" stroke="#203147"/></marker>
</defs>''']
        self.rect(0, 0, width, height, "#ffffff", "none")

    def rect(self, x, y, width, height, fill="white", stroke="#bac6d4", radius=0):
        self.parts.append(f'<rect x="{x}" y="{y}" width="{width}" height="{height}" '
                          f'rx="{radius}" fill="{fill}" stroke="{stroke}"/>')

    def text(self, x, y, value, size=18, anchor="start", bold=False, mono=False, color=INK):
        family = "monospace" if mono else "Arial, sans-serif"
        self.parts.append(f'<text x="{x}" y="{y}" font-family="{family}" font-size="{size}" '
                          f'font-weight="{"bold" if bold else "normal"}" text-anchor="{anchor}" '
                          f'fill="{color}">{escape(value)}</text>')

    def line(self, points, dashed=False, end=None, start=None):
        attrs = ' stroke-dasharray="7 5"' if dashed else ''
        if end:
            attrs += f' marker-end="url(#{end})"'
        if start:
            attrs += f' marker-start="url(#{start})"'
        self.parts.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in points)}" '
                          f'fill="none" stroke="{INK}" stroke-width="1.5"{attrs}/>')

    def box(self, x, y, width, title, fields=(), methods=(), stereotype=None):
        head = 60 if stereotype else 40
        height = head + len(fields) * 23 + (14 if fields else 0) + len(methods) * 23 + 15
        self.rect(x, y, width, height)
        if stereotype:
            self.text(x + width / 2, y + 22, f"«{stereotype}»", 16, "middle", color=MUTED)
        self.text(x + width / 2, y + head - 12, title, 20, "middle", bold=True)
        self.line([(x, y + head), (x + width, y + head)])
        baseline = y + head + 25
        for field in fields:
            self.text(x + 14, baseline, field, 16, mono=True)
            baseline += 23
        if fields:
            self.line([(x, baseline - 9), (x + width, baseline - 9)])
            baseline += 14
        for method in methods:
            self.text(x + 14, baseline, method, 16, mono=True)
            baseline += 23
        return height

    def save(self, name):
        (DESTINATION / name).write_text("\n".join(self.parts + ["</svg>"]) + "\n", encoding="utf-8")


def classes():
    svg = Svg(1480, 1550, "Курьерская служба — UML-диаграмма классов")
    svg.text(40, 42, "Курьерская служба · диаграмма классов", 29, bold=True)
    for y, height, color, title in (
        (75, 155, "#f1f6ff", "1. Пользовательский интерфейс · l1_UserInterface.cpp"),
        (255, 240, "#f0f9f5", "2. Операционный уровень · l2_ApplicationLayer"),
        (520, 485, "#fff8eb", "3. Предметная область · l3_DomainLayer"),
        (1030, 420, "#f6f2fb", "4. Инфраструктура · l4_InfrastructureLayer"),
    ):
        svg.rect(25, y, 1430, height, color, "#d8e0e9", 8)
        svg.text(45, y + 30, title, 20, bold=True)
    svg.box(100, 130, 350, "TerminalOutput", methods=["+ Output(string): void"])
    svg.box(900, 130, 470, "main()", methods=["getline → std::quoted → performCommand"])
    svg.box(100, 315, 350, "IOutput", methods=["+ Output(string): void = 0"], stereotype="interface")
    app_height = svg.box(900, 315, 470, "Application",
                        ["- _out: const IOutput&", "- _col: ItemCollector"],
                        ["+ performCommand(args): bool"])
    order_height = svg.box(100, 595, 620, "Order", [
        "- _order_number: uint32_t", "- _courier_name: string", "- _product_name: string",
        "- _delivery_address: string", "- _delivery_cost_kopecks: uint64_t"], [
        "+ Order(number, courier, product, address, cost)", "# invariant(): bool",
        "+ getOrderNumber(): uint32_t", "+ getCourierName(): const string&",
        "+ getProductName(): const string&", "+ getDeliveryAddress(): const string&",
        "+ getDeliveryCostKopecks(): uint64_t", "+ write(ostream&): bool"])
    collector_height = svg.box(900, 595, 470, "ItemCollector", methods=["+ read(istream&): shared_ptr<ICollectable>"])
    svg.box(100, 1110, 620, "ICollectable", methods=["+ write(ostream&): bool = 0"], stereotype="interface")
    svg.box(900, 1110, 470, "ACollector", ["- _items: vector<shared_ptr<ICollectable>>",
            "- _removed_signs: vector<bool>", "- _removed_count: size_t"], [
            "+ read(istream&): shared_ptr<ICollectable> = 0", "+ getSize(), getItem(), isRemoved()",
            "+ addItem(item), removeItem(index)", "+ updateItem(index, item)", "+ clean()",
            "+ loadCollection(file): bool", "+ saveCollection(file): bool"], stereotype="abstract")
    svg.line([(275, 208), (275, 315)], dashed=True, end="triangle")
    svg.line([(1135, 208), (1135, 315)], dashed=True, end="arrow")
    svg.line([(900, 370), (450, 370)], end="arrow")
    svg.text(665, 354, "вывод через интерфейс", 17, "middle")
    svg.line([(1135, 315 + app_height), (1135, 595)], start="diamond")
    svg.text(1150, 560, "владеет", 17)
    svg.line([(900, 410), (795, 410), (795, 740), (720, 740)], dashed=True, end="arrow")
    svg.text(810, 710, "создаёт", 16)
    svg.line([(1135, 595 + collector_height), (1135, 1110)], end="triangle")
    svg.text(1150, 960, "наследует", 17)
    svg.line([(410, 595 + order_height), (410, 1110)], dashed=True, end="triangle")
    svg.line([(900, 640), (845, 640), (845, 1020), (580, 1020), (580, 595 + order_height)],
             dashed=True, end="arrow")
    svg.text(820, 990, "восстанавливает", 16, "end")
    svg.line([(900, 1180), (720, 1180)], start="aggregate")
    svg.text(810, 1165, "записи", 16, "middle")
    # Keep package captions legible where inheritance lines cross their bands.
    for y, color, title in (
        (75, "#f1f6ff", "1. Пользовательский интерфейс · l1_UserInterface.cpp"),
        (255, "#f0f9f5", "2. Операционный уровень · l2_ApplicationLayer"),
        (520, "#fff8eb", "3. Предметная область · l3_DomainLayer"),
        (1030, "#f6f2fb", "4. Инфраструктура · l4_InfrastructureLayer"),
    ):
        svg.rect(40, y + 9, 700, 28, color, "none")
        svg.text(45, y + 30, title, 20, bold=True)
    svg.text(40, 1491, "Сплошная линия с треугольником — наследование; пунктир с треугольником — реализация интерфейса.", 17)
    svg.text(40, 1520, "Ромб — владение / агрегация; стрелка — использование. Детали стандартной библиотеки сокращены.", 17)
    svg.save("classes.svg")


def update():
    svg = Svg(1480, 1230, "Вариант 5 — последовательность команды update")
    svg.text(35, 42, "Команда update · вариант № 5", 29, bold=True)
    svg.text(35, 74, 'update 0 101 "Сидоров Сидор Сидорович" "Ноутбук" "Москва, ул. Новая, 3" 350.75', 17, mono=True)
    lanes = [
        (35, 300, "#f1f6ff", "1. Интерфейс", "main", "разбор строки"),
        (360, 360, "#f0f9f5", "2. Операционный", "app : Application", "сценарий update"),
        (745, 325, "#fff8eb", "3. Предметная область", None, None),
        (1095, 350, "#f6f2fb", "4. Инфраструктура", "_col : ItemCollector", "ACollector::updateItem"),
    ]
    centers = [185, 540, 907, 1270]
    for (x, width, color, title, name, caption), center in zip(lanes, centers):
        svg.rect(x, 100, width, 940, color, "none", 8)
        svg.text(center, 130, title, 19, "middle", bold=True)
        if name:
            svg.rect(x + 15, 155, width - 30, 65, "white", "#9dabbc", 4)
            svg.text(center, 181, name, 19, "middle", bold=True)
            svg.text(center, 205, caption, 16, "middle", color=MUTED)
        svg.line([(center, 220 if name else 470), (center, 1005)], dashed=True)
    svg.rect(532, 280, 16, 640, "#c3e5d6", "#597b6c")
    svg.rect(899, 470, 16, 105, "#f5d7a0", "#9a7b45")
    svg.rect(1262, 645, 16, 205, "#d7c7e9", "#82679c")

    def message(x1, x2, y, label, dashed=False):
        svg.line([(x1, y), (x2, y)], dashed=dashed, end="arrow")
        svg.text((x1 + x2) / 2, y - 12, label, 17, "middle", mono=True)

    def self_call(x, y, label):
        svg.text(x - 18 if x > 1200 else x + 24, y - 12, label, 16,
                 "end" if x > 1200 else "start")
        svg.line([(x, y), (x + 70, y), (x + 70, y + 30), (x, y + 30)], end="arrow")

    svg.text(185, 254, "аргументы выделены через std::quoted", 16, "middle", color=MUTED)
    message(185, 532, 280, "performCommand(args)")
    self_call(548, 335, "7 аргументов; parseIndex()")
    self_call(548, 405, "createOrder(): стоимость → 35075 коп.")
    svg.rect(770, 440, 275, 60, "white", "#b39462", 4)
    svg.text(907, 466, "order : Order", 19, "middle", bold=True)
    svg.text(907, 489, "новая карточка заказа", 16, "middle", color=MUTED)
    message(548, 770, 470, "«create» Order(...)")
    self_call(915, 535, "invariant()")
    message(899, 548, 595, "shared_ptr<Order>", dashed=True)
    message(548, 1262, 645, "updateItem(0, order)")
    self_call(1278, 710, "индекс, удаление, nullptr")
    self_call(1278, 790, "_items[0] = order")
    message(1262, 548, 870, "void", dashed=True)
    message(532, 185, 920, "true", dashed=True)
    svg.text(185, 968, "ожидание следующей строки", 16, "middle", color=MUTED)
    svg.rect(35, 1060, 1410, 140, "#f7f9fc", "#c6d0dd", 6)
    svg.text(55, 1090, "Обработка ошибок и связь с архитектурой", 20, bold=True)
    svg.text(55, 1120, "Ошибочные данные отклоняются до замены; неверный или удалённый индекс вызывает out_of_range.", 18)
    svg.text(55, 1148, "Application выводит ошибку через IOutput и возвращает false; main завершает программу с кодом 1.", 18)
    svg.text(55, 1176, "На инфраструктурной дорожке показан унаследованный метод ACollector того же объекта _col типа ItemCollector.", 17)
    svg.save("update.svg")


if __name__ == "__main__":
    classes()
    update()
    print("Created doc/classes.svg and doc/update.svg")
