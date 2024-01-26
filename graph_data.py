import matplotlib.pyplot as plt

# Function to parse the data from a line
def parse_line(line):
    values = line.strip().split(';')
    fps = float(values[0].split(':')[1])
    particle_counter = float(values[2].split(':')[1])
    grid_size = int(values[1].split(':')[1])
    thread_count = int(values[5].split(':')[1])
    collision_count = int(values[3].split(':')[1])
    collision_test_count = int(values[4].split(':')[1])
    return fps, particle_counter, grid_size, thread_count, collision_count, collision_test_count


file_path = 'results.txt'
with open(file_path, 'r') as file:
    lines = file.readlines()

fps_values = []
particle_counter_values = []
collision_count_values = []
collision_test_count_values = []

for line in lines:
    fps, particle_counter, grid_size, thread_count, collision_count, collision_test_count = parse_line(line)
    if particle_counter != 0:
        fps_values.append(fps)
        particle_counter_values.append(particle_counter)
        collision_count_values.append(collision_count)
        collision_test_count_values.append(collision_test_count)


plt.figure()
plt.plot(particle_counter_values, fps_values, marker='o')
plt.title(f'FPS vs Particle Count (Grid Size: {grid_size}, Thread Count: {thread_count})')
plt.xlabel('PARTICLE_COUNTER')
plt.ylabel('FPS')
plt.ylim(0, 65)

plt.grid(True)
plt.savefig(f'graphs/fps_vs_particle_counter_plot_{grid_size}x{grid_size}_{thread_count}.png')  # Save as PNG file

collision_ratio_values = [x / y if y != 0 else 0 for x, y in zip(collision_count_values, collision_test_count_values)]
plt.figure()
plt.plot(particle_counter_values, collision_ratio_values)
plt.title(f'Collision Detection Ratio vs Particle Count (Grid Size: {grid_size}, Thread Count: {thread_count})')
plt.xlabel("Particle Count")
plt.ylabel("Collision Detection Ratio")

plt.grid(True)
plt.savefig(f'graphs/collision_test_vs_collision_count_{grid_size}x{grid_size}_{thread_count}.png')