import threading
from ursina import *
from ursina.lights import DirectionalLight
import random
from copy import deepcopy
import math

# TODO add controller support
# TODO add BLE-device support
# TODO add timer

app = Ursina()
window.color = color.dark_gray
mouse.visible = True

# =================== НАСТРОЙКИ ===================
ROT_SPEED   = 90          # скорость вращения (град/сек)
THRESHOLD   = 10          # допуск совпадения (градусов)
LEFT_POS    = (-1.5, 0, 0)
RIGHT_POS   = ( 1.5, 0, 0)
CURRENT_ANGLES = [0, 0, 0]  # [yaw, roll, pitch]
ANGLE_FILE = 'ble_angles.txt'
KEYBOARD_ENABLED = True
ANGLE_DEVICE_ENABLED = False

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

def read_angles_from_file():
    """
    Читает последнюю строку из файла с углами.
    Формат: yaw, roll, pitch
    """
    global CURRENT_ANGLES
    try:
        with open(ANGLE_FILE, 'r') as f:
            lines = f.readlines()
            if lines:
                last_line = lines[-1].strip()
                if last_line:
                    parts = last_line.split(',')
                    if len(parts) == 3:
                        # Конвертируем в числа
                        angles = [float(p.strip()) for p in parts]
                        CURRENT_ANGLES = angles
                        print(f"New angles: {CURRENT_ANGLES}")
    except Exception as e:
        print(f"Error reading angles: {e}")

def ble_reader_thread():
    """Поток для чтения углов из файла"""
    while ANGLE_DEVICE_ENABLED:
        read_angles_from_file()
        time.sleep(0.1)  # 10 раз в секунду

def get_rotation_error(obj1, obj2):
    """
    Вычисляет среднюю ошибку вращения между двумя объектами,
    используя скалярное произведение векторов (самый точный метод).
    """
    # 1. Проверяем направление "носа" (Forward vector)
    # В Ursina forward это (0,0,1) в локальных координатах
    f1 = obj1.forward
    f2 = obj2.forward
    
    # Скалярное произведение векторов дает косинус угла между ними
    # dot = cos(theta). Чем ближе к 1.0, тем меньше угол.
    dot_f = f1.dot(f2)
    # Ограничиваем от -1 до 1, чтобы избежать ошибок math.acos из-за погрешностей float
    dot_f = max(-1, min(1, dot_f))
    error_f = math.degrees(math.acos(dot_f))
    # 2. Проверяем направление "верха" (Up vector), чтобы учесть крен (Roll)
    u1 = obj1.up
    u2 = obj2.up
    dot_u = u1.dot(u2)
    dot_u = max(-1, min(1, dot_u))
    error_u = math.degrees(math.acos(dot_u))
    # Возвращаем среднюю ошибку между направлением носа и верха
    return (error_f + error_u) / 2

# =================== МОДЕЛЬ ===================
# github.com/pokepetter/ursina/blob/master/ursina/models/procedural/cone.py
class Cone(Mesh):
    _cache = {}
    def __new__(cls, resolution=4, radius=.5, height=1, add_bottom=True, mode='triangle'):
        key = (resolution, radius, height, add_bottom, mode)
        if key in cls._cache:
            return deepcopy(cls._cache[key])

        instance = super().__new__(cls)
        cls._cache[key] = instance
        return instance

    def __init__(self, resolution=4, radius=.5, height=1, add_bottom=True, mode='triangle', **kwargs):

        v = Vec3(radius, 0, 0)
        origin = Vec3(0,0,0)
        degrees_to_rotate = 360 / resolution

        verts = []
        for i in range(resolution):
            verts.append(Vec3(v[0], -(height/2), v[1]))
            v = rotate_around_point_2d(v, origin, -degrees_to_rotate)
            verts.append(Vec3(v[0], -(height/2), v[1]))

            verts.append(Vec3(0,height/2,0))
        if add_bottom:
            for i in range(resolution):
                verts.append(Vec3(v[0], 0-(height/2), v[1]))
                verts.append(Vec3(0,-(height/2),0))
                v = rotate_around_point_2d(v, origin, -degrees_to_rotate)
                verts.append(Vec3(v[0], -(height/2), v[1]))


        super().__init__(vertices=verts, uvs=[e.xy for e in verts], mode=mode, **kwargs)

def create_ship(parent, main_col, accent_col):
    """
    Создаёт наглядную асимметричную модельку "самолёта".
    """
    # return Entity(parent=parent, model='blender-monkey.glb', scale=1, color=main_col)

    # Фюзеляж (удлинён по Z — сразу видно, куда смотрит нос)
    body = Entity(parent=parent, model='cube', scale=(0.1, 0.1, 2.2), color=main_col)

    # Нос — яркий, чтобы сразу было видно "перёд"
    nose = Entity(parent=parent, model=Cone(20), scale=(0.3, 0.3, 0.3),
                  position=(0, 0, 1.2), color=accent_col)
    nose.rotation_x = 90

    # Хвостовое оперение
    tail = Entity(parent=parent, model=Cone(2), scale=(0.4, 0.4, 0.1),
                  position=(0, -0.25, -0.9), color=accent_col)
    tail.rotation_y = 90
    tail.rotation_x = 180

    # Крылья (разные по бокам — видно вращение по Roll)
    wings = Entity(parent=parent, model=Cone(2), scale=(1.5, 1, 0.1),
                    position=(0, 0, 0.2), color=accent_col)
    wings.rotation_x = 90

    return body


# =================== СЦЕНА ===================
# Освещение (чтобы были тени и объём)
DirectionalLight(parent=scene, position=(5, 10, 5)).look_at((0, 0, 0))

# --- ЛЕВАЯ сторона (Игрок) ---
player_root = Entity(position=LEFT_POS)
create_ship(player_root, color.red, color.orange)
Text(text='ТЫ (красный)', position=(-0.42, -0.38), origin=(0, 0),
     scale=1.8, color=color.light_gray)

# --- ПРАВАЯ сторона (Образец) ---
target_root = Entity(position=RIGHT_POS)
create_ship(target_root, color.green, color.olive)
Text(text='ЦЕЛЬ (зелёный)', position=(0.42, -0.38), origin=(0, 0),
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
    position=(0, -0.42), origin=(0, 0), scale=1.2
)
status = Text(text='', position=(0, 0.35), origin=(0, 0), scale=2.5, color=color.gold)


# =================== UPDATE ===================
def update():
    # --- Управление ---
    spd = ROT_SPEED * time.dt

    if KEYBOARD_ENABLED:
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

    if ANGLE_DEVICE_ENABLED:
        player_root.rotation_x = -CURRENT_ANGLES[2]  # pitch
        player_root.rotation_y = CURRENT_ANGLES[0]   # yaw
        player_root.rotation_z = CURRENT_ANGLES[1]   # roll

    # --- Проверка совпадения ---
    dx = angle_distance(player_root.rotation_x, target_root.rotation_x)
    dy = angle_distance(player_root.rotation_y, target_root.rotation_y)
    dz = angle_distance(player_root.rotation_z, target_root.rotation_z)

    # Совпали?
    error = get_rotation_error(player_root, target_root)
    if error < THRESHOLD:
        status.text = 'СОВПАЛО!'
        status.color = color.gold
        # Чтобы не вызывалось 100 раз в секунду, можно добавить флаг
        if not hasattr(update, 'won'):
            update.won = True
            invoke(new_round, delay=1.5)
    else:
        update.won = False # сброс флага
        status.text = f'Разница: {error:.1f}°'
        status.color = color.white


# =================== INPUT ===================
def input(key):
    if key == 'escape':
        quit()
    if key == 'space':
        player_root.rotation = (0, 0, 0)   # только сброс своей модели
    if key == 'r':
        new_round()                        # полный новый раунд

threading.Thread(target=ble_reader_thread, daemon=True).start()
app.run()