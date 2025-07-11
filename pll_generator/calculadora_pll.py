def generar_registros(f_mhz):
    f_vco = f_mhz * 32  # Asume divisor ÷32
    int_val = int(f_vco / 10)  # Simulación básica del valor N
    r0 = (int_val << 15) | 1

    r1 = 0x8008011A
    r2 = 0x000004B3
    r3 = 0x00004E42
    r4 = 0x08008011
    r5 = 0x00580000

    print(f"\nFrecuencia deseada: {f_mhz} MHz")
    print(f"R0: {r0:08X}")
    print(f"R1: {r1:08X}")
    print(f"R2: {r2:08X}")
    print(f"R3: {r3:08X}")
    print(f"R4: {r4:08X}")
    print(f"R5: {r5:08X}")

if __name__ == "__main__":
    try:
        f_mhz = float(input("Ingrese frecuencia deseada en MHz (35-4400): "))
        if 35 <= f_mhz <= 4400:
            generar_registros(f_mhz)
        else:
            print("Frecuencia fuera de rango permitido.")
    except ValueError:
        print("Por favor, ingrese un número válido.")
