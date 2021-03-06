static SymTabRec RADEONChipsets[] = {
    { PCI_CHIP_RADEON_QD, "ATI Radeon QD (AGP)" },
    { PCI_CHIP_RADEON_QE, "ATI Radeon QE (AGP)" },
    { PCI_CHIP_RADEON_QF, "ATI Radeon QF (AGP)" },
    { PCI_CHIP_RADEON_QG, "ATI Radeon QG (AGP)" },
    { PCI_CHIP_RV100_QY, "ATI Radeon VE/7000 QY (AGP/PCI)" },
    { PCI_CHIP_RV100_QZ, "ATI Radeon VE/7000 QZ (AGP/PCI)" },
    { PCI_CHIP_RADEON_LW, "ATI Radeon Mobility M7 LW (AGP)" },
    { PCI_CHIP_RADEON_LX, "ATI Mobility FireGL 7800 M7 LX (AGP)" },
    { PCI_CHIP_RADEON_LY, "ATI Radeon Mobility M6 LY (AGP)" },
    { PCI_CHIP_RADEON_LZ, "ATI Radeon Mobility M6 LZ (AGP)" },
    { PCI_CHIP_RS100_4136, "ATI Radeon IGP320 (A3) 4136" },
    { PCI_CHIP_RS100_4336, "ATI Radeon IGP320M (U1) 4336" },
    { PCI_CHIP_RS200_4137, "ATI Radeon IGP330/340/350 (A4) 4137" },
    { PCI_CHIP_RS200_4337, "ATI Radeon IGP330M/340M/350M (U2) 4337" },
    { PCI_CHIP_RS250_4237, "ATI Radeon 7000 IGP (A4+) 4237" },
    { PCI_CHIP_RS250_4437, "ATI Radeon Mobility 7000 IGP 4437" },
    { PCI_CHIP_R200_QH, "ATI FireGL 8700/8800 QH (AGP)" },
    { PCI_CHIP_R200_QL, "ATI Radeon 8500 QL (AGP)" },
    { PCI_CHIP_R200_QM, "ATI Radeon 9100 QM (AGP)" },
    { PCI_CHIP_R200_BB, "ATI Radeon 8500 AIW BB (AGP)" },
    { PCI_CHIP_R200_BC, "ATI Radeon 8500 AIW BC (AGP)" },
    { PCI_CHIP_RV200_QW, "ATI Radeon 7500 QW (AGP/PCI)" },
    { PCI_CHIP_RV200_QX, "ATI Radeon 7500 QX (AGP/PCI)" },
    { PCI_CHIP_RV250_If, "ATI Radeon 9000/PRO If (AGP/PCI)" },
    { PCI_CHIP_RV250_Ig, "ATI Radeon 9000 Ig (AGP/PCI)" },
    { PCI_CHIP_RV250_Ld, "ATI FireGL Mobility 9000 (M9) Ld (AGP)" },
    { PCI_CHIP_RV250_Lf, "ATI Radeon Mobility 9000 (M9) Lf (AGP)" },
    { PCI_CHIP_RV250_Lg, "ATI Radeon Mobility 9000 (M9) Lg (AGP)" },
    { PCI_CHIP_RS300_5834, "ATI Radeon 9100 IGP (A5) 5834" },
    { PCI_CHIP_RS300_5835, "ATI Radeon Mobility 9100 IGP (U3) 5835" },
    { PCI_CHIP_RS350_7834, "ATI Radeon 9100 PRO IGP 7834" },
    { PCI_CHIP_RS350_7835, "ATI Radeon Mobility 9200 IGP 7835" },
    { PCI_CHIP_RV280_5960, "ATI Radeon 9200PRO 5960 (AGP)" },
    { PCI_CHIP_RV280_5961, "ATI Radeon 9200 5961 (AGP)" },
    { PCI_CHIP_RV280_5962, "ATI Radeon 9200 5962 (AGP)" },
    { PCI_CHIP_RV280_5964, "ATI Radeon 9200SE 5964 (AGP)" },
    { PCI_CHIP_RV280_5C61, "ATI Radeon Mobility 9200 (M9+) 5C61 (AGP)" },
    { PCI_CHIP_RV280_5C63, "ATI Radeon Mobility 9200 (M9+) 5C63 (AGP)" },
    { PCI_CHIP_R300_AD, "ATI Radeon 9500 AD (AGP)" },
    { PCI_CHIP_R300_AE, "ATI Radeon 9500 AE (AGP)" },
    { PCI_CHIP_R300_AF, "ATI Radeon 9600TX AF (AGP)" },
    { PCI_CHIP_R300_AG, "ATI FireGL Z1 AG (AGP)" },
    { PCI_CHIP_R300_ND, "ATI Radeon 9700 Pro ND (AGP)" },
    { PCI_CHIP_R300_NE, "ATI Radeon 9700/9500Pro NE (AGP)" },
    { PCI_CHIP_R300_NF, "ATI Radeon 9700 NF (AGP)" },
    { PCI_CHIP_R300_NG, "ATI FireGL X1 NG (AGP)" },
    { PCI_CHIP_RV350_AP, "ATI Radeon 9600 AP (AGP)" },
    { PCI_CHIP_RV350_AQ, "ATI Radeon 9600SE AQ (AGP)" },
    { PCI_CHIP_RV360_AR, "ATI Radeon 9600XT AR (AGP)" },
    { PCI_CHIP_RV350_AS, "ATI Radeon 9600 AS (AGP)" },
    { PCI_CHIP_RV350_AT, "ATI FireGL T2 AT (AGP)" },
    { PCI_CHIP_RV350_AV, "ATI FireGL RV360 AV (AGP)" },
    { PCI_CHIP_RV350_NP, "ATI Radeon Mobility 9600/9700 (M10/M11) NP (AGP)" },
    { PCI_CHIP_RV350_NQ, "ATI Radeon Mobility 9600 (M10) NQ (AGP)" },
    { PCI_CHIP_RV350_NR, "ATI Radeon Mobility 9600 (M11) NR (AGP)" },
    { PCI_CHIP_RV350_NS, "ATI Radeon Mobility 9600 (M10) NS (AGP)" },
    { PCI_CHIP_RV350_NT, "ATI FireGL Mobility T2 (M10) NT (AGP)" },
    { PCI_CHIP_RV350_NV, "ATI FireGL Mobility T2e (M11) NV (AGP)" },
    { PCI_CHIP_R350_AH, "ATI Radeon 9800SE AH (AGP)" },
    { PCI_CHIP_R350_AI, "ATI Radeon 9800 AI (AGP)" },
    { PCI_CHIP_R350_AJ, "ATI Radeon 9800 AJ (AGP)" },
    { PCI_CHIP_R350_AK, "ATI FireGL X2 AK (AGP)" },
    { PCI_CHIP_R350_NH, "ATI Radeon 9800PRO NH (AGP)" },
    { PCI_CHIP_R350_NI, "ATI Radeon 9800 NI (AGP)" },
    { PCI_CHIP_R350_NK, "ATI FireGL X2 NK (AGP)" },
    { PCI_CHIP_R360_NJ, "ATI Radeon 9800XT NJ (AGP)" },
    { PCI_CHIP_RV380_3E50, "ATI Radeon X600 (RV380) 3E50 (PCIE)" },
    { PCI_CHIP_RV380_3E54, "ATI FireGL V3200 (RV380) 3E54 (PCIE)" },
    { PCI_CHIP_RV380_3150, "ATI Radeon Mobility X600 (M24) 3150 (PCIE)" },
    { PCI_CHIP_RV380_3154, "ATI FireGL M24 GL 3154 (PCIE)" },
    { PCI_CHIP_RV370_5B60, "ATI Radeon X300 (RV370) 5B60 (PCIE)" },
    { PCI_CHIP_RV370_5B62, "ATI Radeon X600 (RV370) 5B62 (PCIE)" },
    { PCI_CHIP_RV370_5B64, "ATI FireGL V3100 (RV370) 5B64 (PCIE)" },
    { PCI_CHIP_RV370_5B65, "ATI FireGL D1100 (RV370) 5B65 (PCIE)" },
    { PCI_CHIP_RV370_5460, "ATI Radeon Mobility M300 (M22) 5460 (PCIE)" },
    { PCI_CHIP_RV370_5464, "ATI FireGL M22 GL 5464 (PCIE)" },
    { PCI_CHIP_R420_JH, "ATI Radeon X800 (R420) JH (AGP)" },
    { PCI_CHIP_R420_JI, "ATI Radeon X800PRO (R420) JI (AGP)" },
    { PCI_CHIP_R420_JJ, "ATI Radeon X800SE (R420) JJ (AGP)" },
    { PCI_CHIP_R420_JK, "ATI Radeon X800 (R420) JK (AGP)" },
    { PCI_CHIP_R420_JL, "ATI Radeon X800 (R420) JL (AGP)" },
    { PCI_CHIP_R420_JM, "ATI FireGL X3 (R420) JM (AGP)" },
    { PCI_CHIP_R420_JN, "ATI Radeon Mobility 9800 (M18) JN (AGP)" },
    { PCI_CHIP_R420_JP, "ATI Radeon X800XT (R420) JP (AGP)" },
    { PCI_CHIP_R423_UH, "ATI Radeon X800 (R423) UH (PCIE)" },
    { PCI_CHIP_R423_UI, "ATI Radeon X800PRO (R423) UI (PCIE)" },
    { PCI_CHIP_R423_UJ, "ATI Radeon X800LE (R423) UJ (PCIE)" },
    { PCI_CHIP_R423_UK, "ATI Radeon X800SE (R423) UK (PCIE)" },
    { PCI_CHIP_R423_UQ, "ATI FireGL V7200 (R423) UQ (PCIE)" },
    { PCI_CHIP_R423_UR, "ATI FireGL V5100 (R423) UR (PCIE)" },
    { PCI_CHIP_R423_UT, "ATI FireGL V7100 (R423) UT (PCIE)" },
    { PCI_CHIP_R423_5D57, "ATI Radeon X800XT (R423) 5D57 (PCIE)" },

    { -1,                 NULL }
};
