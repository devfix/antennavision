# Agenda

## Software
- [x] look for setup files in a directory, loop over all setups
- [x] implement plot_gain_over_straight
- [x] implement ULA
- [x] check setup timestamp -> update only if new
- [x] fix cylinder width in three: line2
- [x] remove fmt dependency finally
- [x] add StandingWaveDipole to factory::make_radiator
- [x] implement beamwidth measure function
- [x] pre compute radiation resistance for gain and directivtiy
- [x] add UPA
- [ ] add beamforming coefficients
- [ ] check angle between axis1 and axis2 at plane definiton -> error if not 90 deg
- [ ] implement PS
- [ ] implement connections between components
- [ ] add spdlog
- parallel plane or better spherical plane
- find beam areas
- codebook: different planes with normalized distances to UPA
- low resolution phase shifters

## Writing
- [ ] derive magnetic vector potential using Lorenz Equation
- [ ] derive effective length vector and express field quantities by it
- [ ] derive beamforming model for PS and TDD
- [ ] derive beam focusing (x,y)
- [ ] E focus vs "P focus" (P variation) -> compare Hertzian Dipole vs lambda/2 dipole