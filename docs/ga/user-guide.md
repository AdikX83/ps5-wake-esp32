# PS5 Wake – treoir úsáideora

Is clár beag ESP32-C6 é PS5 Wake a chasann do PlayStation 5 air nuair a fhaigheann sé paicéad Wake-on-LAN nó nuair a bhrúnn tú a chnaipe. Thar Bluetooth, ligeann sé air gurb é do rialtóir DualSense péireáilte é – díreach cosúil le brú ar an gcnaipe PS. Oibríonn sé ó mhód scíthe agus ó chonsól atá múchta go hiomlán.

## Cad atá de dhíth ort

- Clár ESP32-C6 le PS5 Wake (tástáilte: ESP32-C6-DevKitM-1), faoi chumhacht ó luchtaire USB ar bith.
- Rialtóir DualSense (nó DualSense Edge) atá péireáilte le do PS5 cheana.
- Ríomhaire agus cábla USB **sonraí** – uair amháin, chun an dochtearraí a shuiteáil agus seoltaí an rialtóra a léamh.
- Líonra Wi-Fi 2.4 GHz.

## 1. An dochtearraí a shuiteáil

Oscail an suiteálaí gréasáin (nasc in README an tionscadail) in Chrome nó Edge, ceangail an clár le cábla USB agus cliceáil **Install**. Lasann gléas nua **corcra**. Tosaíonn sé líonra an phainéil socraithe leis féin.

## 2. An painéal socraithe a oscailt

1. Coinnigh an cnaipe **BOOT** síos ar feadh 5–20 soicind agus scaoil é. Lasann an LED **dearg**. Ar ghléas nua tá an painéal ar siúl cheana (LED corcra) – scipeáil an chéim seo.
2. Ar an ríomhaire a gceanglóidh tú an rialtóir leis, ceangail leis an líonra Wi-Fi `PS5-WAKE-xxxx`, pasfhocal `ps5wol-setup`.
3. Sa bhrabhsálaí oscail **`https://192.168.4.1`** (nó `https://ps5-wake-xxxx.local`). Tabharfaidh an brabhsálaí rabhadh nach bhfuil an ceangal príobháideach, mar úsáideann an ESP32 a theastas féin – roghnaigh **Ardleibhéal → Lean ar aghaidh** (braitheann na hainmneacha ar an mbrabhsálaí).
4. Roghnaigh do theanga ag barr an leathanaigh.

## 3. Seoltaí an rialtóra a léamh

1. Ceangail an DualSense leis an ríomhaire leis an gcábla USB.
2. Sa chuid **Bluetooth** den phainéal, íoslódáil an léitheoir seoltaí:
   - **Windows:** `ps5-wake-reader.cmd` – cliceáil faoi dhó air. Má thugann Windows rabhadh faoi fhoilsitheoir anaithnid, roghnaigh **Tuilleadh faisnéise → Rith ar aon nós**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Taispeánann an léitheoir seoltaí Bluetooth an chonsóil agus an rialtóra agus cóipeálann sé **cód socraithe** (`PS5WAKE:…`) chuig an ngearrthaisce. Greamaigh é sa réimse **Cód socraithe ón léitheoir seoltaí** – líontar an dá sheoladh.

Ní athraíonn na seoltaí ach amháin má phéireálann tú an rialtóir leis an gconsól arís.

## 4. An socrú a chríochnú

1. Roghnaigh do líonra Wi-Fi le **Scan**, cuir isteach a phasfhocal agus roghnaigh do thír.
2. Cuir isteach **Pasfhocal nua do líonra an phainéil** (8 gcarachtar ar a laghad). Tá sé riachtanach, mar tá an pasfhocal réamhshocraithe poiblí.
3. Cliceáil **Sábháil agus atosaigh**. Dúntar an painéal, caochann an LED **gorm** le linn an cheangail agus lasann sé **glas** nuair a bhíonn gach rud réidh.

## 5. Wake-on-LAN a shocrú

- I d'aip Wake-on-LAN (nó ar leathanach WoL do ródaire) úsáid **Seoladh MAC an ESP32 seo** ón bpainéal, port UDP **9** agus **seoladh craolta** do líonra (m.sh. `192.168.1.255`).
- Cuir seoladh IP seasta in áirithe don ESP32 i do ródaire.
- Má úsáideann d'aip nó do ródaire **seoladh MAC líonra an PS5 féin** cheana, is féidir leat é a chur isteach sa réimse **Freagair seoladh MAC an PS5 féin freisin** – freagróidh an ESP32 dó freisin. I líonraí mogalra (m.sh. ASUS AiMesh) b'fhéidir nach sroichfidh paicéid den sórt sin an ESP32; sa chás sin seol WoL chuig MAC an ESP32.

## Úsáid laethúil

- **Staid an chonsóil:** bíonn an ponc ar an deilbhín cumhachta ag barr an phainéil glas nuair atá an PS5 ar siúl, oráiste i mód scíthe agus dearg nuair atá sé múchta nó as raon (liath: níor aimsíodh fós). Ní aithnítear mód scíthe ach amháin má fhanann an consól ceangailte leis an idirlíon.
- **An PS5 a dhúiseacht:** seol paicéad Wake-on-LAN nó brúigh **BOOT** go gairid. Splancann an LED bán uair amháin, caochann sé cian le linn na hiarrachta, agus ansin splancann sé **glas 3×** (d'fhreagair an consól) nó **dearg 3×** (gan freagra).
- Bíonn 30 soicind ar a laghad idir iarrachtaí; tugann iarratas a ndearnadh neamhaird de splanc oráiste amháin.

## An cnaipe BOOT

| Fad a choinneáil | LED le linn a choinneáil | Toradh |
|---|---|---|
| faoi 1 s | – | an PS5 a dhúiseacht |
| 1–5 s | bán | faic (cealaigh) |
| 5–20 s, ansin scaoil | dearg (caochann ó 15 s) | an painéal socraithe a oscailt |
| 20 s | corcra, ag caochadh go tapa | gach socrú a scriosadh agus atosú |

Má choinníonn tú BOOT síos agus an chumhacht á ceangal, tosaíonn mód sábháilte gan Wi-Fi ná Bluetooth (LED buí).

## Dathanna an LED

| Dath | Iompar | Brí |
|---|---|---|
| corcra | seasta | gan socrú – tá an painéal socraithe ar siúl |
| gorm | ag caochadh go mall | ag ceangal le Wi-Fi |
| oráiste | ag caochadh go mall | Wi-Fi socraithe ach gan cheangal le breis is 30 s |
| glas (lag) | seasta | réidh (is féidir é a mhúchadh sa phainéal) |
| dearg | seasta | tá an painéal socraithe ar siúl |
| buí | seasta | mód sábháilte |
| bán | splanc amháin | fuarthas Wake-on-LAN nó brúdh BOOT |
| cian | ag caochadh go tapa | iarracht dúiseachta ar siúl |
| glas | trí splanc | d'fhreagair an consól |
| dearg | trí splanc | níor fhreagair an consól |
| oráiste | splanc amháin | rinneadh neamhaird den iarratas (iarracht ar siúl nó sos 30 s) |

## An painéal socraithe

- Múchann an painéal agus a líonra tar éis sábhála, tar éis **Dún gan athruithe** nó tar éis 10 nóiméad gan ghníomhaíocht. Tugann **Cuir leis** tuilleadh ama.
- **Múch an LED go hiomlán**: Gan dathanna, caochadh ná splancanna ar chor ar bith. Ach agus BOOT á choinneáil agat, taispeánann an LED fad an bhrú fós.
- **Ainm an ghléis (roghnach)**: M.sh. Seomra suí. Úsáidtear é i seoladh an phainéil (`seomra-sui.local`), i Home Assistant agus i dteideal an leathanaigh – úsáideach le níos mó ná gléas amháin.
- **Rochtain ón líonra baile**: Ar siúl de réir réamhshocraithe. Beidh an leathanach ar fáil i gcónaí ar do líonra baile ag `<IP an ESP32>` agus `ps5-wake-xxxx.local` – logáil isteach le pasfhocal an phainéil (ainm úsáideora admin mura bhfuil sé athraithe agat). Ní thosaítear líonra Wi-Fi an phainéil; coinnigh BOOT ar feadh 5 s chun é a oscailt.
- **Ceangal criptithe (HTTPS)**: Molta. Cruthaíonn an ESP32 a theastas féin, mar sin ar an gcéad chuairt tugann an brabhsálaí rabhadh nach bhfuil an ceangal príobháideach – cuir an méarlorg thíos i gcomparáid agus lean ar aghaidh. Gan HTTPS, seoltar an pasfhocal agus na socruithe gan chriptiú ar do líonra. Athnuaitear an teastas go huathoibríoch sula rachaidh sé in éag (faigheann an ESP32 an t-am ón idirlíon); cruthaíonn **Teastas nua** ceann nua láithreach.
- Déanann **Múscail an PS5 anois** tástáil ar an dúiseacht ón bpainéal.

## Fabhtcheartú

- **An LED ag caochadh oráiste:** pasfhocal Wi-Fi mícheart, líonra 5 GHz amháin nó comhartha lag. Oscail an painéal, seiceáil an Wi-Fi agus bog an ESP32 níos gaire don ródaire. Atosaíonn an ESP32 leis féin má bhíonn sé as líne ar feadh 2 nóiméad.
- **Ní tharlaíonn aon rud tar éis Wake-on-LAN a sheoladh (gan splanc bhán):** ní shroicheann an paicéad an ESP32. Úsáid MAC an ESP32, an seoladh craolta agus port 9; seiceáil nach bhfuil an ESP32 ar líonra aoi.
- **Splancanna dearga tar éis iarrachta:** seiceáil na seoltaí Bluetooth (léigh arís iad má phéireáil tú an rialtóir arís) agus bog an ESP32 níos gaire don chonsól.
- **Ní aimsíonn an léitheoir an rialtóir:** úsáid cábla sonraí (ní cábla luchtaithe amháin) agus ceangail an rialtóir go díreach leis an ríomhaire.
- **Tugann an brabhsálaí rabhadh faoin teastas:** tá sé seo le súil leis – criptítear an painéal le teastas an ESP32 féin. Lean ar aghaidh chuig an leathanach; taispeánann an painéal méarlorg an teastais.
