from ursina import *
from ursina.lights import DirectionalLight
import random

app = Ursina()
window.color = color.dark_gray
mouse.visible = True

# =================== НАСТРОЙКИ ===================
ROT_SPEED   = 90          # скорость вращения (град/сек)
THRESHOLD   = 10          # допуск совпадения (градусов)
LEFT_POS    = (-1.5, 0, 0)
RIGHT_POS   = ( 1.5, 0, 0)


# =================== УТИЛИТЫ ===================
def norm_angle(a):
    """Приводит угол к диапазону [-180, 180]"""
    a = a % 360
    if a > 180:
        a -= 360
    return a

def angle_distance(a, b):
    """Минимальное расстояние между двумя углами"""
    return abs(norm_angle((a % 360) - (b % 360)))


# =================== МОДЕЛЬ ===================
def create_ship(parent, main_col, accent_col):
    """
    Создаёт наглядную асимметричную модельку "самолёта".
    """
    # return Entity(parent=parent, model='blender-monkey.glb', scale=1, color=main_col)

    # Фюзеляж (удлинён по Z — сразу видно, куда смотрит нос)
    body = Entity(parent=parent, model='cube', scale=(0.7, 0.5, 2.2), color=main_col)

    # Нос — яркий, чтобы сразу было видно "перёд"
    nose = Entity(parent=parent, model='cube', scale=(0.5, 0.4, 0.9),
                  position=(0, 0, 1.4), color=accent_col)

    # Хвостовое оперение
    tail = Entity(parent=parent, model='cube', scale=(0.9, 0.08, 0.5),
                  position=(0, 0.25, -1.0), color=accent_col)

    # Крылья (разные по бокам — видно вращение по Roll)
    wing_l = Entity(parent=parent, model='cube', scale=(1.3, 0.06, 0.45),
                    position=(-0.65, 0, 0.2), color=accent_col)
    wing_r = Entity(parent=parent, model='cube', scale=(1.3, 0.06, 0.45),
                    position=(0.65, 0, 0.2), color=accent_col)

    return body


# =================== СЦЕНА ===================
# Освещение (чтобы были тени и объём)
DirectionalLight(parent=scene, position=(5, 10, 5)).look_at((0, 0, 0))

# --- ЛЕВАЯ сторона (Игрок) ---
player_root = Entity(position=LEFT_POS)
create_ship(player_root, color.red, color.orange)
Text(text='ТЫ (красный)', position=(-0.32, -0.42), origin=(0, 0),
     scale=1.8, color=color.light_gray)

# --- ПРАВАЯ сторона (Образец) ---
target_root = Entity(position=RIGHT_POS)
create_ship(target_root, color.green, color.lime)
Text(text='ЦЕЛЬ (зелёный)', position=(0.32, -0.42), origin=(0, 0),
     scale=1.8, color=color.light_gray)

# Камера смотрит ровно на центр между ними
camera.position = (0, 1.5, 11)
camera.look_at((0, 0, 0))


# =================== ЛОГИКА ===================
def new_round():
    """Задаёт новый случайный поворот цели и сбрасывает игрока"""
    rx = random.randint(0, 359)
    ry = random.randint(0, 359)
    rz = random.randint(0, 359)
    target_root.rotation = (rx, ry, rz)
    player_root.rotation = (0, 0, 0)   # каждый раунд начинаем с нуля

new_round()

# UI-подсказки
hint = Text(
    text='W / S — вращать по X\nA / D — вращать по Y\nQ / E — вращать по Z\n'
         'Пробел — сбросить свой корабль    R — новый раунд',
    position=(0, -0.48), origin=(0, 0), scale=1.2
)
status = Text(text='', position=(0, 0.35), origin=(0, 0), scale=2.5, color=color.gold)


# =================== UPDATE ===================
def update():
    # --- Управление ---
    spd = ROT_SPEED * time.dt

    if held_keys['w'] or held_keys['up arrow']:
        player_root.rotation_x -= spd
    if held_keys['s'] or held_keys['down arrow']:
        player_root.rotation_x += spd
    if held_keys['a'] or held_keys['left arrow']:
        player_root.rotation_y -= spd
    if held_keys['d'] or held_keys['right arrow']:
        player_root.rotation_y += spd
    if held_keys['q']:
        player_root.rotation_z += spd
    if held_keys['e']:
        player_root.rotation_z -= spd

    # --- Проверка совпадения ---
    dx = angle_distance(player_root.rotation_x, target_root.rotation_x)
    dy = angle_distance(player_root.rotation_y, target_root.rotation_y)
    dz = angle_distance(player_root.rotation_z, target_root.rotation_z)

    # Совпали?
    if dx < THRESHOLD and dy < THRESHOLD and dz < THRESHOLD:
        status.text = '✅ ИДЕАЛЬНО!'
        status.color = color.gold
        invoke(new_round, delay=1.2) # TODO constant rotation
    else:
        # Показываем, насколько близко (опционально, можно убрать)
        avg = (dx + dy + dz) / 3
        status.text = f'Разница: {avg:.1f}°'
        status.color = color.white


# =================== INPUT ===================
def input(key):
    if key == 'escape':
        quit()
    if key == 'space':
        player_root.rotation = (0, 0, 0)   # только сброс своей модели
    if key == 'r':
        new_round()                        # полный новый раунд

app.run()