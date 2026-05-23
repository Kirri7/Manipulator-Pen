from ursina import *
from ursina.prefabs.first_person_controller import FirstPersonController

app = Ursina()

# Настройки
target_rotation = (45, 30, 60)  # Целевое вращение (в градусах X, Y, Z)
threshold = 0.10  # Порог точности совпадения

# Разделим экран: слева игрок, справа пример
split_screen = Entity(parent=camera.ui, model='quad', scale=(1, 0.9), color=color.clear)

# === ЛЕВАЯ ЧАСТЬ (Игрок) ===
player_camera = Entity(camera=True, position=(0, 0, 10), parent=scene)
player_scene = Entity(parent=scene, x=-2)
player_model = Entity(parent=player_scene, model='cube', color=color.red, scale=2)

# === ПРАВАЯ ЧАСТЬ (Образец) ===
target_camera = Entity(camera=True, position=(0, 0, 10), parent=scene)
target_scene = Entity(parent=scene, x=2)
target_model = Entity(parent=target_scene, model='cube', color=color.green, scale=2)
target_model.rotation = target_rotation

# Текст подсказки
hint = Text(text='WASD/Стрелки — поворот\nЦель: совпасть с зеленым кубом', position=(-0.8, -0.45), origin=(0, 0), scale=1.5)
success = Text(text='', position=(0, 0), origin=(0, 0), scale=2, color=color.red)

def update():
    # Управление: WASD / Стрелочки
    speed_modif = 20
    angle = speed_modif * time.dt  # Скорость поворота

    if held_keys['w'] or held_keys['up arrow']:
        player_model.rotation_x += angle
    if held_keys['s'] or held_keys['down arrow']:
        player_model.rotation_x -= angle
    if held_keys['a'] or held_keys['left arrow']:
        player_model.rotation_y += angle
    if held_keys['d'] or held_keys['right arrow']:
        player_model.rotation_y -= angle
    if held_keys['q']:
        player_model.rotation_z += angle
    if held_keys['e']:
        player_model.rotation_z -= angle

    # Проверка победы (через кватернионы для точности)
    diff = player_model.rotation - target_rotation
    # Простое сравнение углов (для прототипа)
    if all(abs(d) < 5 for d in diff):
        success.text = '✅ ПОБЕДА! Поворачиваю снова...'
        success.color = color.g
        reset_target()
    else:
        success.text = ''

def reset_target():
    global target_rotation
    target_rotation = random.rotation().as_euler()
    target_model.rotation = target_rotation

def input(key):
    if key == 'escape':
        quit()
    if key == 'space':
        reset_target()  # Новый раунд

app.run()