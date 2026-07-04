# Agenda

## Software
- [x] look for setup files in a directory, loop over all setups
- [x] implement plot_gain_over_straight
- [x] implement ULA
- [x] check setup timestamp -> update only if new
- [x] fix cylinder width in three: line2
- [x] remove fmt dependency finally
- [ ] add beamforming coefficients
- [ ] add StandingWaveDipole to factory::make_radiator
- [ ] check angle between axis1 and axis2 at plane definiton -> error if not 90 deg
- [ ] implement beamwidth measure function
- [ ] implement PS
- [ ] implement connections between components
- [ ] pre compute radiation resistance for gain and directivtiy
- [ ] add spdlog

## Writing
- [ ] derive magnetic vector potential using Lorenz Equation
- [ ] derive effective length vector and express field quantities by it
- [ ] derive beamforming model for PS and TDD
- [ ] derive beam focusing (x,y)
- [ ] E focus vs "P focus" (P variation) -> compare Hertzian Dipole vs lambda/2 dipole