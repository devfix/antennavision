import numpy as np
import matplotlib.pyplot as plt
import json
import scipy.constants as const

INFINITY = float(np.finfo(np.float32).max)

def generiere_dft_codebook(n_antennas, oversampling_faktor=1):
    """
    Generiert ein DFT-basiertes Codebook für ein Uniform Linear Array (ULA).

    Parameter:
    n_antennas (int): Anzahl der Antennenelemente (N).
    oversampling_faktor (int): Faktor für das Oversampling (O).
                               Bei O=1 entsteht ein Basis-DFT-Codebook (orthogonale Beams).
                               Bei O>1 überlappen sich die Beams, um den Quantisierungsfehler zu minimieren.

    Rückgabe:
    numpy.ndarray: Eine Matrix der Größe (n_antennas x (n_antennas * oversampling_faktor)),
                   wobei jede Spalte einen normierten Beamforming-Vektor darstellt.
    """
    N = n_antennas
    M = N * oversampling_faktor  # Gesamtanzahl der Vektoren im Codebook

    # Indizes für Antennenelemente (n) und Codebook-Einträge (m)
    n = np.arange(N)
    m = np.arange(M)

    # Vektorisierte Berechnung der Phasenmatrix mithilfe des äußeren Produkts
    # Formel: exp(j * 2 * pi * n * m / (N * O))
    phasen_matrix = 1j * 2 * np.pi * np.outer(n, m) / M

    # Erstelle das Codebook und normiere die Vektoren (Leistung = 1)
    codebook = np.exp(phasen_matrix) / np.sqrt(N)

    return codebook

def berechne_array_faktor(codebook, n_antennas, distance):
    """
    Berechnet den Array-Faktor (das Abstrahldiagramm) für alle Vektoren im Codebook
    über alle räumlichen Winkel von -90 bis +90 Grad.
    """
    # Winkel von -90 bis 90 Grad in Radiant
    theta_grad = np.linspace(-90, 90, 1000)
    theta_rad = np.radians(theta_grad)

    # Erstelle die theoretisch perfekten Steering-Vektoren für jeden Winkel
    # Abstand d ist als Vielfaches der Wellenlänge angegeben (Standard: 0.5 * lambda)
    n = np.arange(n_antennas)
    # Formel für räumliche Phasenverschiebung: 2 * pi * d/lambda * sin(theta)
    steering_matrix = np.exp(1j * 2 * np.pi * distance * np.outer(n, np.sin(theta_rad)))

    # Das Abstrahldiagramm ergibt sich aus dem Skalarprodukt der Codebook-Vektoren
    # mit den theoretischen Steering-Vektoren.
    # Wir nehmen den Betrag und wandeln ihn in Dezibel (dB) um.
    array_faktor = np.abs(np.dot(codebook.conj().T, steering_matrix))

    # Vermeide log(0) durch Hinzufügen einer winzigen Zahl
    array_faktor_db = 20 * np.log10(array_faktor + 1e-10)

    return theta_grad, array_faktor_db

def plotte_codebook(theta_grad, array_faktor_db, titel):
    """Hilfsfunktion zum Plotten des Antennendiagramms."""
    plt.figure(figsize=(10, 6))

    # Zeichne das Muster jedes Vektors im Codebook
    for i in range(array_faktor_db.shape[0]):
        plt.plot(theta_grad, array_faktor_db[i, :], label=f'Beam {i}')

    plt.title(titel, fontsize=14)
    plt.xlabel('Angle (Grad)', fontsize=12)
    plt.ylabel('Gain (dB)', fontsize=12)
    plt.ylim(-20, 20)
    plt.xlim(-90, 90)
    plt.grid(True, linestyle='--', alpha=0.7)
    # plt.legend(loc='upper right', fontsize='small', ncol=2) # Legend bei vielen Beams optional ausblenden
    plt.tight_layout()
    plt.show()

def speichere_codebook_json(codebook, n_antennas, wavelengths: list[float], distance, oversampling: int, dateiname="codebook.json"):
    """
    Speichert das Codebook in einer strukturierten JSON-Datei.
    Unterstützt eine oder mehrere Frequenzen, Richtung und speichert komplexe Zahlen als Dictionaries.
    """

    M = codebook.shape[1] # Anzahl der Beams im Codebook

    # Grundgerüst der JSON-Datei
    json_daten = {
        "array_type": "ULA",
        "n_elements": n_antennas,
        "oversampling_factor": oversampling,
        "distance": distance,
        "wavelengths": wavelengths,
        "description": "DFT-Codebook",
        "codebook": {} # Dictionary: Frequenz-Index (als String) -> Liste von Beams
    }

    for i, wavelength in enumerate(wavelengths):
        wavelength_key = str(i)  # JSON-Schlüssel müssen Strings sein (Nutze den Index aus der Liste)
        json_daten["codebook"][wavelength_key] = {}

        for m in range(M):
            # Berechne den theoretischen Hauptstrahl-Winkel (Fernfeld) für diesen Vektor
            m_shift = m if m <= M//2 else m - M
            sin_theta = (m_shift / M) * (wavelength / distance)

            # Überprüfen, ob der Winkel im physikalisch sichtbaren Bereich liegt
            if abs(sin_theta) <= 1.0:
                winkel_grad = np.arcsin(sin_theta)
            else:
                winkel_grad = None # Außerhalb des sichtbaren Bereichs (z. B. wenn lambda extrem groß ist)
                print("skipping")
                continue

            vektor = codebook[:, m]

            weights = [[float(np.abs(v)), float(np.angle(v))] for v in vektor]

            eintrag = {
                "angle": winkel_grad,
                "focus_distance": INFINITY,
                "weights": weights
            }
            json_daten["codebook"][wavelength_key][str(m)] = eintrag

    with open(dateiname, 'w', encoding='utf-8') as f:
        json.dump(json_daten, f, indent=4)
    print(f"Codebook erfolgreich als '{dateiname}' gespeichert (Wavelengths: {wavelengths}).")

# ==========================================
# Hauptprogramm (Ausführung)
# ==========================================
if __name__ == "__main__":
    n_antennas = 16
    wavelengths = [0.05, 0.1, 0.15]
    wavelength_center = np.mean(wavelengths)
    distance = wavelength_center / 2.0
    print(f'element distance: {distance:.03f} m')

    print(f"Generiere Codebooks für ein lineares Array mit {n_antennas} Antennen...")

    # 1. Basis DFT-Codebook (Oversampling = 1)
    codebook_basis = generiere_dft_codebook(n_antennas, oversampling_faktor=1)
    print(f"Basis-Codebook generiert. Dimension: {codebook_basis.shape} (Antennen x Beams)")

    theta_basis, af_basis = berechne_array_faktor(codebook_basis, n_antennas, distance / wavelength_center)
    plotte_codebook(theta_basis, af_basis, f'DFT-Codebook (N={n_antennas}, Oversampling=1)')

    # NEU: Speichern in JSON (mit mehreren Frequenzen)
    speichere_codebook_json(codebook_basis, n_antennas, wavelengths, distance, 1, "dft_basis_codebook.json")

    # 2. Oversampled DFT-Codebook (Oversampling = 4, typisch für LTE/5G)
    O_faktor = 4
    codebook_oversampled = generiere_dft_codebook(n_antennas, oversampling_faktor=O_faktor)
    print(f"Oversampled-Codebook generiert. Dimension: {codebook_oversampled.shape} (Antennen x Beams)")

    theta_over, af_over = berechne_array_faktor(codebook_oversampled, n_antennas, distance / wavelength_center)
    plotte_codebook(theta_over, af_over, f'DFT-Codebook (N={n_antennas}, Oversampling={O_faktor})')

    # NEU: Speichern in JSON (mit mehreren Frequenzen)
    speichere_codebook_json(codebook_oversampled, n_antennas, wavelengths, distance, O_faktor, "dft_oversampled_codebook.json")
