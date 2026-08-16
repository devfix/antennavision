//
// Created by Tristan Krause on 2026-06-21.
//

#include <nlohmann/json.hpp>
#include <print>
#include <vector>
#include "math/coords.hpp"
#include "builtin.hpp"

namespace builtin
{
    namespace
    {
        ojson const js = ojson::parse(R"JSON(
{
  "metadata": {
    "setup_name": "ula-beamwidth"
  },
  "variables": {
    "system_wavelength": 0.1,
    "wavelength": "system_wavelength",
    "distance": 10000,
    "dipole_length": "0.5 * wavelength"
  },
  "references": [
    {
      "id": "ref_rx",
      "origin": ""
    }
  ],
  "antennas": [
    {
      "id": "tx",
      "type": "UPA",
      "ref": "",
      "spacing_x": "wavelength * 0.5",
      "spacing_y": "wavelength * 0.5",
      "size_x": 8,
      "size_y": 16,
      "radiator": {
        "type": "StandingWaveDipole",
        "dipole_length": "dipole_length"
      }
    },
    {
      "id": "rx",
      "ref": "ref_rx",
      "type": "StandingWaveDipole",
      "dipole_length": "dipole_length"
    }
  ]
}
)JSON");
    }

    BUILTIN_FUNCTION(t01_upa_beam_shape, setup::Setup& setup_task)
    {
        setup::Setup const setup(js);
        setup.export_to_three(".");

    }
} // namespace builtin
