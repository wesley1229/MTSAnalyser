# MTSAnalyser
MPEG-2 Transport Stream PSI analyzer developed by Wesley

/********************************************
*      MPEG-2 Transport Stream PSI analyzer *
*      Wesley Chen 2013-2020          *
*      Software Verion 1.1.0           *
********************************************/

MPEG-2 Transport Stream PSI analyser is the software tool to help you to analyze standard MPEG-2 PSI information, SCTE65/PSIP, SCTE18/PSIP information
It includes several features as below description.

Feature:
1. support below standard PSI/SI parsing

	MPEG-2 standard PSI: 
	                                PAT,PMT,CAT,TSDT
	SCTE65/PSIP main profile: 
	                            NIT-CDS, NIT-MMS, NTT-SNS, SVCT-DCM, SVCT-VCM, SVCT-ICM, STT
	SCTE18/PSIP: 
	             EAS
	ATSC/PSIP:
		         EAS

2. MPEG-2 standard PSI version upgrade.
3. MPEG-2, SCTE 65/P1/P2/P3, SCTE 18 descriptor process.
4. Muti-section process.
5. DSM-CC/ObjectCarousel data parsing.
6. Support dynamic virutal channel detection.
7. Easy porting to different platform.

Know issue:
1. ATSC/PSIP,DVB-SI not yet implemented.
2. Caption descriptor not yet implemented.
