// __BEGIN_LICENSE__
//  Copyright (c) 2009-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NGT platform is licensed under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance with the
//  License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


/// \file Colormap.cc

#include <vw/Image/Colormap.h>

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>

namespace vw { namespace cm {

// Black-body colormap.
//http://www.kennethmoreland.com/color-advice/
const char *BlackBody = R""""(
0	0	0	0
0.015873016	13	4	2
0.031746032	22	8	4
0.047619048	29	12	7
0.063492063	34	15	9
0.079365079	40	17	11
0.095238095	46	18	13
0.111111111	53	20	15
0.126984127	59	21	16
0.142857143	65	23	18
0.158730159	72	24	19
0.174603175	79	25	20
0.19047619	86	26	21
0.206349206	93	27	22
0.222222222	99	28	23
0.238095238	107	29	24
0.253968254	114	30	25
0.26984127	121	31	26
0.285714286	128	31	27
0.301587302	135	32	28
0.317460317	143	32	29
0.333333333	150	33	30
0.349206349	158	33	31
0.365079365	165	34	32
0.380952381	173	34	33
0.396825397	179	37	34
0.412698413	183	44	33
0.428571429	188	51	32
0.444444444	192	57	30
0.46031746	196	63	29
0.476190476	200	68	28
0.492063492	204	74	26
0.507936508	208	80	24
0.523809524	212	85	21
0.53968254	216	90	18
0.555555556	220	95	15
0.571428571	224	101	10
0.587301587	227	106	5
0.603174603	228	114	8
0.619047619	229	121	10
0.634920635	230	128	13
0.650793651	230	135	15
0.666666667	231	141	18
0.682539683	232	148	21
0.698412698	232	155	23
0.714285714	232	161	26
0.73015873	233	168	28
0.746031746	233	174	31
0.761904762	233	180	33
0.777777778	233	187	36
0.793650794	233	193	38
0.80952381	233	199	41
0.825396825	232	206	43
0.841269841	232	212	46
0.857142857	231	218	48
0.873015873	231	224	51
0.888888889	230	230	56
0.904761905	237	234	92
0.920634921	242	237	122
0.936507937	246	240	150
0.952380952	250	244	176
0.968253968	253	248	203
0.984126984	254	251	229
1	255	255	255
)"""";

// Viridis colormap.
//http://www.kennethmoreland.com/color-advice/
const char *Viridis = R""""(
0	68	1	84
0.015873016	70	7	90
0.031746032	71	13	96
0.047619048	71	19	101
0.063492063	72	24	107
0.079365079	72	30	111
0.095238095	72	35	116
0.111111111	72	40	120
0.126984127	71	45	124
0.142857143	70	50	127
0.158730159	69	55	130
0.174603175	67	60	132
0.19047619	66	65	134
0.206349206	64	70	136
0.222222222	62	74	137
0.238095238	60	79	138
0.253968254	58	83	139
0.26984127	56	87	140
0.285714286	54	92	141
0.301587302	52	96	141
0.317460317	50	100	142
0.333333333	49	104	142
0.349206349	47	108	142
0.365079365	45	112	142
0.380952381	44	116	142
0.396825397	42	119	142
0.412698413	41	123	142
0.428571429	39	127	142
0.444444444	38	131	142
0.46031746	36	135	142
0.476190476	35	138	141
0.492063492	33	142	141
0.507936508	32	146	140
0.523809524	31	150	139
0.53968254	31	154	138
0.555555556	31	157	137
0.571428571	31	161	135
0.587301587	33	165	134
0.603174603	35	169	132
0.619047619	38	172	129
0.634920635	42	176	127
0.650793651	47	180	124
0.666666667	53	183	121
0.682539683	59	187	117
0.698412698	66	190	113
0.714285714	74	194	109
0.73015873	82	197	105
0.746031746	90	200	100
0.761904762	99	203	95
0.777777778	108	206	89
0.793650794	118	208	84
0.80952381	128	211	77
0.825396825	138	213	71
0.841269841	148	216	65
0.857142857	159	218	58
0.873015873	169	220	51
0.888888889	180	221	44
0.904761905	191	223	37
0.920634921	202	224	31
0.936507937	212	226	26
0.952380952	223	227	24
0.968253968	233	228	26
0.984126984	243	230	30
1	253	231	37
)"""";

// Plasma colormap.
//http://www.kennethmoreland.com/color-advice/
const char *Plasma = R""""(
0	13	8	135
0.015873016	25	6	140
0.031746032	34	6	144
0.047619048	42	5	147
0.063492063	50	5	151
0.079365079	57	4	154
0.095238095	64	4	156
0.111111111	70	3	159
0.126984127	77	2	161
0.142857143	84	2	163
0.158730159	90	1	165
0.174603175	96	1	166
0.19047619	103	0	167
0.206349206	109	0	168
0.222222222	115	1	168
0.238095238	121	2	168
0.253968254	127	3	168
0.26984127	133	6	167
0.285714286	139	10	165
0.301587302	145	14	163
0.317460317	150	19	161
0.333333333	156	23	158
0.349206349	161	28	155
0.365079365	166	32	152
0.380952381	171	37	148
0.396825397	176	41	145
0.412698413	180	46	141
0.428571429	185	50	137
0.444444444	189	55	133
0.46031746	193	59	130
0.476190476	197	64	126
0.492063492	201	69	122
0.507936508	205	73	118
0.523809524	209	78	115
0.53968254	212	82	111
0.555555556	216	87	108
0.571428571	219	92	104
0.587301587	222	96	101
0.603174603	225	101	97
0.619047619	228	106	94
0.634920635	231	111	90
0.650793651	234	116	87
0.666666667	237	121	83
0.682539683	239	126	80
0.698412698	242	131	76
0.714285714	244	136	73
0.73015873	246	141	69
0.746031746	248	147	66
0.761904762	249	153	62
0.777777778	250	158	59
0.793650794	252	164	55
0.80952381	253	170	52
0.825396825	253	176	49
0.841269841	254	182	46
0.857142857	254	188	43
0.873015873	253	195	40
0.888888889	253	201	38
0.904761905	252	208	37
0.920634921	251	214	36
0.936507937	249	221	37
0.952380952	247	228	37
0.968253968	245	235	39
0.984126984	242	242	39
1	240	249	33
)"""";

// Inferno colormap.
//http://www.kennethmoreland.com/color-advice/
const char *Inferno = R""""(
0	0	0	4
0.015873016	2	1	10
0.031746032	4	3	19
0.047619048	7	5	27
0.063492063	11	7	36
0.079365079	16	9	46
0.095238095	21	11	56
0.111111111	27	12	65
0.126984127	34	12	75
0.142857143	40	11	84
0.158730159	48	10	91
0.174603175	55	9	97
0.19047619	62	9	102
0.206349206	69	10	105
0.222222222	75	12	107
0.238095238	82	14	109
0.253968254	88	16	110
0.26984127	95	19	110
0.285714286	101	21	110
0.301587302	108	24	110
0.317460317	114	26	110
0.333333333	120	28	109
0.349206349	127	30	108
0.365079365	133	33	107
0.380952381	140	35	105
0.396825397	146	37	104
0.412698413	153	40	101
0.428571429	159	42	99
0.444444444	165	45	96
0.46031746	172	47	93
0.476190476	178	50	90
0.492063492	184	53	87
0.507936508	190	56	83
0.523809524	196	60	79
0.53968254	201	64	75
0.555555556	207	68	70
0.571428571	212	72	66
0.587301587	217	77	61
0.603174603	221	82	57
0.619047619	226	87	52
0.634920635	230	93	47
0.650793651	234	99	42
0.666666667	237	105	37
0.682539683	240	111	32
0.698412698	243	118	27
0.714285714	245	125	21
0.73015873	247	132	16
0.746031746	249	139	11
0.761904762	250	146	7
0.777777778	251	154	6
0.793650794	252	161	8
0.80952381	252	169	14
0.825396825	252	177	21
0.841269841	251	185	30
0.857142857	250	193	39
0.873015873	249	201	49
0.888888889	247	209	60
0.904761905	246	217	72
0.920634921	244	224	85
0.936507937	242	232	100
0.952380952	241	239	116
0.968253968	243	245	133
0.984126984	246	250	150
1	252	255	164
)"""";

// Kindlmann colormap.
//http://www.kennethmoreland.com/color-advice/
const char *Kindlmann = R""""(
0	0	0	0
0.015873016	17	1	17
0.031746032	27	1	29
0.047619048	33	2	42
0.063492063	36	3	54
0.079365079	38	3	66
0.095238095	39	4	78
0.111111111	39	4	90
0.126984127	38	5	104
0.142857143	36	6	117
0.158730159	32	6	131
0.174603175	28	7	144
0.19047619	25	7	156
0.206349206	17	8	169
0.222222222	8	18	175
0.238095238	8	31	173
0.253968254	8	43	167
0.26984127	8	53	159
0.285714286	7	62	150
0.301587302	7	70	141
0.317460317	6	76	133
0.333333333	6	83	127
0.349206349	6	88	120
0.365079365	6	94	115
0.380952381	5	99	110
0.396825397	5	104	106
0.412698413	5	109	102
0.428571429	5	115	97
0.444444444	6	120	92
0.46031746	6	125	86
0.476190476	6	130	78
0.492063492	6	135	71
0.507936508	7	140	62
0.523809524	7	145	53
0.53968254	7	150	43
0.555555556	7	155	32
0.571428571	8	159	21
0.587301587	8	164	11
0.603174603	16	169	8
0.619047619	24	174	8
0.634920635	35	178	9
0.650793651	48	182	9
0.666666667	63	186	9
0.682539683	79	190	9
0.698412698	95	193	9
0.714285714	112	196	9
0.73015873	129	198	10
0.746031746	146	200	10
0.761904762	164	202	10
0.777777778	181	204	10
0.793650794	198	205	10
0.80952381	216	205	10
0.825396825	235	205	11
0.841269841	247	205	97
0.857142857	250	208	146
0.873015873	251	212	172
0.888888889	252	217	190
0.904761905	253	222	204
0.920634921	253	227	215
0.936507937	254	233	225
0.952380952	254	238	233
0.968253968	254	244	241
0.984126984	255	249	248
1	255	255	255
)"""";
  
// CubeHelix colormap.
//https://www.mrao.cam.ac.uk/~dag/CUBEHELIX/
std::string CubeHelix[] = {
"0% 0	0	0",
"0.392157%% 1.85519 0.448284 1.62689",
"0.784314%% 3.65524 0.911975 3.32151",
"1.17647% 5.39735	1.39316	5.08029",
"1.56863% 7.07892	1.89387	6.89953",
"1.96078% 8.69752	2.41603	8.77538",
"2.35294% 10.2509	2.96153	10.7039",
"2.7451% 11.7371	3.53215	12.6809",
"3.13725% 13.1543	4.12958	14.7023",
"3.52941% 14.5009	4.75545	16.7637",
"3.92157% 15.7756	5.41128	18.8608",
"4.31373% 16.977	6.09849	20.9891",
"4.70588% 18.1043	6.81841	23.1441",
"5.09804% 19.1568	7.57228	25.3213",
"5.4902% 20.1338	8.36121	27.5159",
"5.88235% 21.0352	9.18623	29.7234",
"6.27451% 21.8608	10.0483	31.9392",
"6.66667% 22.6107	10.9481	34.1586",
"7.05882% 23.2852	11.8864	36.3768",
"7.45098% 23.885	12.8639	38.5894",
"7.84314% 24.4106	13.8809	40.7917",
"8.23529% 24.8632	14.9379	42.979",
"8.62745% 25.2439	16.0351	45.147",
"9.01961% 25.5539	17.1726	47.2911",
"9.41176% 25.795	18.3504	49.407",
"9.80392% 25.9687	19.5686	51.4904",
"10.1961% 26.077	20.8269	53.537",
"10.5882% 26.1221	22.1249	55.5428",
"10.9804% 26.1061	23.4623	57.5037",
"11.3725% 26.0316	24.8386	59.4158",
"11.7647% 25.9011	26.2532	61.2755",
"12.1569% 25.7174	27.7052	63.0791",
"12.549% 25.4834	29.194	64.8231",
"12.9412% 25.2021	30.7185	66.5042",
"13.3333% 24.8767	32.2777	68.1193",
"13.7255% 24.5105	33.8706	69.6654",
"14.1176% 24.1069	35.4959	71.1397",
"14.5098% 23.6694	37.1523	72.5395",
"14.902% 23.2017	38.8384	73.8624",
"15.2941% 22.7073	40.5528	75.1061",
"15.6863% 22.1902	42.2939	76.2687",
"16.0784% 21.6542	44.0601	77.3482",
"16.4706% 21.1031	45.8497	78.3429",
"16.8627% 20.541	47.6611	79.2516",
"17.2549% 19.9719	49.4922	80.0728",
"17.6471% 19.3998	51.3414	80.8057",
"18.0392% 18.8288	53.2067	81.4493",
"18.4314% 18.2629	55.086	82.0032",
"18.8235% 17.7064	56.9775	82.467",
"19.2157% 17.1632	58.8789	82.8406",
"19.6078% 16.6375	60.7883	83.1239",
"20% 16.1332	62.7036	83.3174",
"20.3922% 15.6545	64.6225	83.4215",
"20.7843% 15.2053	66.5429	83.437",
"21.1765% 14.7895	68.4627	83.3648",
"21.5686% 14.4111	70.3796	83.2061",
"21.9608% 14.0737	72.2915	82.9624",
"22.3529% 13.7812	74.1962	82.635",
"22.7451% 13.5371	76.0915	82.226",
"23.1373% 13.3451	77.9752	81.7372",
"23.5294% 13.2086	79.8451	81.1708",
"23.9216% 13.1308	81.6992	80.5292",
"24.3137% 13.1151	83.5352	79.815",
"24.7059% 13.1645	85.3512	79.031",
"25.098% 13.2819	87.145	78.18",
"25.4902% 13.4703	88.9147	77.2651",
"25.8824% 13.7323	90.6583	76.2895",
"26.2745% 14.0704	92.3739	75.2568",
"26.6667% 14.4869	94.0595	74.1704",
"27.0588% 14.9842	95.7135	73.0339",
"27.451% 15.5643	97.3339	71.8513",
"27.8431% 16.2289	98.9192	70.6264",
"28.2353% 16.9799	100.468	69.3634",
"28.6275% 17.8187	101.978	68.0662",
"29.0196% 18.7468	103.448	66.7393",
"29.4118% 19.7651	104.878	65.387",
"29.8039% 20.8748	106.264	64.0136",
"30.1961% 22.0765	107.607	62.6236",
"30.5882% 23.3708	108.906	61.2216",
"30.9804% 24.7582	110.158	59.8122",
"31.3725% 26.2388	111.363	58.3999",
"31.7647% 27.8126	112.521	56.9896",
"32.1569% 29.4794	113.63	55.5857",
"32.549% 31.2389	114.69	54.193",
"32.9412% 33.0903	115.7	52.8163",
"33.3333% 35.033	116.66	51.46",
"33.7255% 37.0658	117.569	50.129",
"34.1176% 39.1878	118.428	48.8278",
"34.5098% 41.3975	119.236	47.5609",
"34.902% 43.6933	119.992	46.333",
"35.2941% 46.0735	120.698	45.1484",
"35.6863% 48.5362	121.352	44.0116",
"36.0784% 51.0794	121.956	42.9269",
"36.4706% 53.7007	122.51	41.8984",
"36.8627% 56.3978	123.014	40.9304",
"37.2549% 59.168	123.469	40.0268",
"37.6471% 62.0086	123.875	39.1916",
"38.0392% 64.9167	124.234	38.4284",
"38.4314% 67.8893	124.545	37.741",
"38.8235% 70.9232	124.811	37.1329",
"39.2157% 74.0151	125.032	36.6074",
"39.6078% 77.1615	125.209	36.1677",
"40% 80.3589	125.343	35.817",
"40.3922% 83.6035	125.436	35.5579",
"40.7843% 86.8918	125.49	35.3934",
"41.1765% 90.2197	125.505	35.3258",
"41.5686% 93.5833	125.484	35.3576",
"41.9608% 96.9785	125.428	35.4908",
"42.3529% 100.401	125.338	35.7275",
"42.7451% 103.848	125.217	36.0693",
"43.1373% 107.313	125.066	36.5179",
"43.5294% 110.793	124.888	37.0746",
"43.9216% 114.284	124.683	37.7406",
"44.3137% 117.781	124.456	38.5167",
"44.7059% 121.28	124.206	39.4037",
"45.098% 124.777	123.937	40.4021",
"45.4902% 128.266	123.65	41.5122",
"45.8824% 131.744	123.349	42.7341",
"46.2745% 135.207	123.035	44.0675",
"46.6667% 138.65	122.709	45.5122",
"47.0588% 142.069	122.376	47.0676",
"47.451% 145.459	122.037	48.7328",
"47.8431% 148.816	121.694	50.5069",
"48.2353% 152.137	121.349	52.3886",
"48.6275% 155.417	121.006	54.3765",
"49.0196% 158.652	120.666	56.4691",
"49.4118% 161.839	120.331	58.6644",
"49.8039% 164.972	120.005	60.9604",
"50.1961% 168.05	119.688	63.3548",
"50.5882% 171.067	119.384	65.8454",
"50.9804% 174.022	119.095	68.4294",
"51.3725% 176.909	118.823	71.1041",
"51.7647% 179.727	118.571	73.8665",
"52.1569% 182.471	118.339	76.7136",
"52.549% 185.139	118.131	79.6419",
"52.9412% 187.729	117.949	82.6481",
"53.3333% 190.237	117.794	85.7285",
"53.7255% 192.661	117.669	88.8795",
"54.1176% 194.999	117.576	92.0972",
"54.5098% 197.249	117.515	95.3775",
"54.902% 199.407	117.49	98.7165",
"55.2941% 201.474	117.501	102.11",
"55.6863% 203.447	117.551	105.553",
"56.0784% 205.324	117.641	109.042",
"56.4706% 207.105	117.772	112.572",
"56.8627% 208.788	117.946	116.139",
"57.2549% 210.373	118.164	119.738",
"57.6471% 211.859	118.428	123.364",
"58.0392% 213.244	118.738	127.012",
"58.4314% 214.53	119.095	130.679",
"58.8235% 215.716	119.501	134.358",
"59.2157% 216.802	119.957	138.046",
"59.6078% 217.788	120.462	141.737",
"60% 218.675	121.018	145.427",
"60.3922% 219.464	121.625	149.111",
"60.7843% 220.155	122.284	152.784",
"61.1765% 220.749	122.994	156.442",
"61.5686% 221.249	123.757	160.079",
"61.9608% 221.654	124.573	163.691",
"62.3529% 221.967	125.44	167.274",
"62.7451% 222.19	126.36	170.824",
"63.1373% 222.325	127.332	174.335",
"63.5294% 222.373	128.355	177.803",
"63.9216% 222.338	129.431	181.225",
"64.3137% 222.221	130.556	184.595",
"64.7059% 222.025	131.733	187.911",
"65.098% 221.754	132.958	191.169",
"65.4902% 221.409	134.233	194.364",
"65.8824% 220.994	135.555	197.493",
"66.2745% 220.512	136.925	200.553",
"66.6667% 219.967	138.34	203.54",
"67.0588% 219.362	139.8	206.451",
"67.451% 218.7	141.303	209.284",
"67.8431% 217.985	142.848	212.036",
"68.2353% 217.221	144.434	214.703",
"68.6275% 216.412	146.06	217.284",
"69.0196% 215.562	147.722	219.777",
"69.4118% 214.673	149.421	222.179",
"69.8039% 213.752	151.154	224.489",
"70.1961% 212.801	152.919	226.704",
"70.5882% 211.824	154.715	228.825",
"70.9804% 210.827	156.54	230.849",
"71.3725% 209.813	158.391	232.775",
"71.7647% 208.786	160.268	234.604",
"72.1569% 207.75	162.167	236.333",
"72.549% 206.71	164.087	237.963",
"72.9412% 205.67	166.025	239.495",
"73.3333% 204.634	167.98	240.927",
"73.7255% 203.605	169.949	242.26",
"74.1176% 202.589	171.93	243.496",
"74.5098% 201.59	173.921	244.633",
"74.902% 200.61	175.92	245.675",
"75.2941% 199.655	177.924	246.621",
"75.6863% 198.727	179.932	247.473",
"76.0784% 197.831	181.941	248.233",
"76.4706% 196.971	183.948	248.902",
"76.8627% 196.15	185.953	249.483",
"77.2549% 195.371	187.951	249.978",
"77.6471% 194.637	189.943	250.388",
"78.0392% 193.953	191.924	250.717",
"78.4314% 193.32	193.894	250.967",
"78.8235% 192.743	195.85	251.142",
"79.2157% 192.223	197.79	251.244",
"79.6078% 191.764	199.713	251.276",
"80% 191.368	201.615	251.242",
"80.3922% 191.037	203.497	251.146",
"80.7843% 190.773	205.355	250.99",
"81.1765% 190.579	207.188	250.779",
"81.5686% 190.456	208.994	250.516",
"81.9608% 190.406	210.772	250.207",
"82.3529% 190.43	212.521	249.853",
"82.7451% 190.53	214.238	249.461",
"83.1373% 190.706	215.923	249.034",
"83.5294% 190.96	217.574	248.576",
"83.9216% 191.292	219.191	248.091",
"84.3137% 191.702	220.771	247.585",
"84.7059% 192.192	222.315	247.061",
"85.098% 192.76	223.821	246.525",
"85.4902% 193.408	225.288	245.98",
"85.8824% 194.134	226.716	245.431",
"86.2745% 194.937	228.104	244.883",
"86.6667% 195.818	229.453	244.34",
"87.0588% 196.776	230.76	243.807",
"87.451% 197.808	232.027	243.288",
"87.8431% 198.915	233.253	242.787",
"88.2353% 200.093	234.437	242.308",
"88.6275% 201.342	235.581	241.857",
"89.0196% 202.66	236.684	241.437",
"89.4118% 204.045	237.747	241.052",
"89.8039% 205.494	238.77	240.707",
"90.1961% 207.004	239.753	240.404",
"90.5882% 208.574	240.697	240.149",
"90.9804% 210.201	241.603	239.944",
"91.3725% 211.881	242.472	239.794",
"91.7647% 213.612	243.304	239.7",
"92.1569% 215.39	244.101	239.668",
"92.549% 217.211	244.864	239.699",
"92.9412% 219.074	245.594	239.796",
"93.3333% 220.973	246.292	239.963",
"93.7255% 222.906	246.96	240.202",
"94.1176% 224.867	247.599	240.515",
"94.5098% 226.854	248.211	240.904",
"94.902% 228.863	248.797	241.37",
"95.2941% 230.889	249.36	241.917",
"95.6863% 232.928	249.901	242.545",
"96.0784% 234.976	250.422	243.255",
"96.4706% 237.029	250.925	244.049",
"96.8627% 239.082	251.413	244.927",
"97.2549% 241.131	251.886	245.89",
"97.6471% 243.172	252.348	246.938",
"98.0392% 245.2	252.8	248.071",
"98.4314% 247.211	253.245	249.29",
"98.8235% 249.201	253.686	250.592",
"99.2157% 251.165	254.123	251.979",
"99.6078% 253.1	254.561	253.449",
"100% 255	255	255"
};

// Binary-red-blue colormap
std::string BRB[] = {
"0%       59  76 192",
"3.13%    68  90 204",
"6.25%    78 104 215",
"9.38%    88 117 225",
"12.50%   98 130 234",
"15.63%  108 142 241",
"18.75%  119 154 247",
"21.88%  130 165 251",
"25.00%  141 176 254",
"28.13%  152 185 255",
"31.25%  163 194 255",
"34.38%  174 201 253",
"37.50%  184 208 249",
"40.63%  194 213 244",
"43.75%  204 217 238",
"46.88%  213 219 230",
"50.00%  221 221 221",
"53.13%  229 216 209",
"56.25%  236 211 198",
"59.38%  241 204 185",
"62.50%  245 196 173",
"65.63%  247 187 160",
"68.75%  247 177 148",
"71.88%  247 166 135",
"75.00%  244 154 123",
"78.13%  241 141 111",
"81.25%  236 127  99",
"84.38%  229 112  87",
"87.50%  222  97  76",
"90.63%  213  80  66",
"93.75%  203  62  56",
"96.88%  192  40  47",
"100.00% 180   4  38"
};

// Jet colormap
std::string Jet[] = {
"0%    0   0   0",   // Black
"20.8% 0   0   255", // Blue
"25%   0   0   255", // Blue
"37.5% 0   191 255", // Light blue
"41.7% 0   255 255", // Teal
"58.3% 255 255 51",  // Yellow
"62.5% 255 191 0",   // Orange
"75%   255 0   0",   // Red
"79.1% 255 0   0",   // Red
"100%   0  0   0"    // Black
};
  
// Append a line to lut
void AddLutLine(std::string const& line, lut_type & lut){
  typedef boost::tokenizer<> tokenizer;
  boost::char_delimiters_separator<char> sep(false,",: \t");

  tokenizer tokens(line,sep);
  tokenizer::iterator iter = tokens.begin();

  std::string key;
  Vector3u value;

  try {
    // Parse a file having lines of four numbers, e.g., 75 255 0 0
    if (iter == tokens.end()) vw_throw(IOErr() << "Unable to read input LUT");
    key = *iter; iter++;
    if (iter == tokens.end()) vw_throw(IOErr() << "Unable to read input LUT");
    value[0] = boost::numeric_cast<uint8>(round(boost::lexical_cast<double>(*iter))); iter++;
    if (iter == tokens.end()) vw_throw(IOErr() << "Unable to read input LUT");
          value[1] = boost::numeric_cast<uint8>(round(boost::lexical_cast<double>(*iter))); iter++;
          if (iter == tokens.end()) vw_throw(IOErr() << "Unable to read input LUT");
          value[2] = boost::numeric_cast<uint8>(round(boost::lexical_cast<double>(*iter)));
  } catch (const boost::bad_lexical_cast& e) {
    return;
  }
  lut.push_back(lut_element(key, value));
}

// Given a multi-row string, on each row having a value in [0, 1] and 3
// values representing an RGB color, collect these in a lut_map.
// See example input BlackBody above. 
void parse_lut_map(std::string const& table, std::map<float, Vector3u> & lut_map) {
  lut_map.clear();
  std::istringstream is(table);
  double inten, r, g, b;
  while (is >> inten >> r >> g >> b)
    lut_map[inten] = Vector3u(r, g, b);
}

// Populate a colormap from string pairs given in lut.
void populate_lut_map(lut_type const& lut,
                      std::map<float, Vector3u> & lut_map) {

  // First find the min and max keys. This will be used only if we don't
  // have percentages.
  double min_val = std::numeric_limits<double>::max();
  double max_val = -min_val;
  for (size_t it = 0; it < lut.size(); it++) {
    auto const& pair = lut[it];
    double val = atof(pair.first.c_str());
    min_val = std::min(min_val, val);
    max_val = std::max(max_val, val);
  }

  for (size_t it = 0; it < lut.size(); it++) {

    auto const& pair = lut[it];
    try {
      if (boost::contains(pair.first,"%")) {
        // Remove the percent sign and divide by a 100
        float key = boost::lexical_cast<float>(boost::erase_all_copy(pair.first,"%"))/100.0;
        lut_map[key] = pair.second;
      } else {
        float key = boost::lexical_cast<float>(pair.first);
        lut_map[ (key - min_val) / (max_val - min_val) ] =
          pair.second;
      }
    } catch (const boost::bad_lexical_cast& e) {
      continue;
    }
  }
}
  
// Parse the colormap for given style. Note that if colormap_style
// is not one of the supported options, it can be a file name,
// from which the style is read. The output will be either in lut
// or lut_map, depending on the desired style. This will be sorted
// out later.
void parse_color_style(std::string const& colormap_style,
                      std::map<float, Vector3u> & lut_map) {

  lut_type lut;
  lut_map.clear();
  
  // Decide legend
  if (colormap_style == "jet") {
    for (size_t count = 0; count < sizeof(Jet)/sizeof(std::string); count++)
      AddLutLine(Jet[count], lut);
  } else if (colormap_style == "binary-red-blue") {
    for (size_t count = 0; count < sizeof(BRB)/sizeof(std::string); count++)
      AddLutLine(BRB[count], lut);
  } else if (colormap_style == "cubehelix") {
    for (size_t count = 0; count < sizeof(CubeHelix)/sizeof(std::string); count++)
      AddLutLine(CubeHelix[count], lut);
  } else if (colormap_style == "black-body") {
    // Populate directly lut_map
    parse_lut_map(BlackBody, lut_map);
  } else if (colormap_style == "viridis") {
    // Populate directly lut_map
    parse_lut_map(Viridis, lut_map);
  } else if (colormap_style == "plasma") {
    // Populate directly lut_map
    parse_lut_map(Plasma, lut_map);
  } else if (colormap_style == "inferno") {
    // Populate directly lut_map
    parse_lut_map(Inferno, lut_map);
  } else if (colormap_style == "kindlmann") {
    // Populate directly lut_map
    parse_lut_map(Kindlmann, lut_map);
  } else {
    // Read input LUT
    std::ifstream lut_file(colormap_style.c_str());
    if (!lut_file.is_open())
      vw_throw(IOErr() << "Unable to open colormap style file: "
               << colormap_style);
    std::string line;
    std::getline(lut_file, line);
    while (lut_file.good()) {
      
      // Skip lines containing spaces only
      bool only_spaces = true;
      for (unsigned s = 0; s < line.size(); s++)
        if (!isspace(line[s])) only_spaces = false;
      if (only_spaces){
        std::getline(lut_file, line);
        continue;
      }
      
      AddLutLine(line, lut);
      std::getline(lut_file, line);
    }
    
    lut_file.close();
  }

  // If lut_map is not populated so far, convert lut to lut_map
  // (converts altitudes to relative percent).
  if (lut_map.empty())
    populate_lut_map(lut, lut_map);
}

// Colormap function
PixelMask<PixelRGB<uint8>> ColormapFunc::operator()(PixelMask<PixelGray<float>> const& pix) const {
  if (is_transparent(pix))
    return PixelMask<PixelRGB<uint8>>(); // Skip transparent pixels
  
  float val = compound_select_channel<const float&>(pix, 0);
  if (val > 1.0) val = 1.0;
  if (val < 0.0) val = 0.0;
  
  // Get locations on sparse colormap that bound this pixel value
  map_type::const_iterator bot = m_colormap.upper_bound(val); bot--;
  map_type::const_iterator top = m_colormap.upper_bound(val);

  // If this is above the top colormap value, use the max val.
  if (top == m_colormap.end())
    return PixelRGB<uint8>(bot->second[0], bot->second[1], bot->second[2]); 
  
  // Otherwise determine a proportional color between the bracketing
  // colormap values. Do the operations in double precision, then
  // round.
  double ratio = (val - bot->first)/(top->first - bot->first);
  Vector3u output = bot->second + 
    round(ratio * (Vector3(top->second) - Vector3(bot->second)));
  
  return PixelRGB<uint8>(output[0], output[1], output[2]);
}
  
}} //end namespace vw::cm
