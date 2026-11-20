# PS5 Wake – guia do utilizador

O PS5 Wake é uma pequena placa ESP32-C6 que liga a sua PlayStation 5 quando recebe um pacote Wake-on-LAN ou quando
prime o botão da placa. Por Bluetooth, apresenta-se como o seu comando DualSense emparelhado – tal como premir o botão
PS. Funciona a partir do modo de repouso e com a consola completamente desligada.

## O que é necessário

- Uma placa ESP32-C6 com PS5 Wake (testada: ESP32-C6-DevKitM-1), alimentada por qualquer carregador USB.
- Um comando DualSense (ou DualSense Edge) já emparelhado com a PS5.
- Um computador e um cabo USB **de dados** – apenas uma vez, para instalar o firmware e ler os endereços do comando.
- Uma rede Wi-Fi de 2,4 GHz.

## 1. Instalar o firmware

Abra o instalador web (link no README do projeto) no Chrome ou no Edge, ligue a placa com um cabo USB e clique em
**Install or update**. Um dispositivo novo acende a **roxo**. Inicia sozinho a rede do painel de configuração.

## 2. Abrir o painel de configuração

1. Mantenha o botão **BOOT** premido durante 5–20 segundos e solte-o. O LED fica **vermelho**. Num dispositivo novo o painel já está ativo (LED roxo) – salte este passo.
2. No computador a que vai ligar o comando, ligue-se à rede Wi-Fi **`PS5-WAKE-xxxx`**, palavra-passe
   **`ps5wol-setup`**.
3. No navegador abra **`https://192.168.4.1`** (ou `https://ps5-wake-xxxx.local`). O navegador avisa que a ligação não é privada, porque o ESP32 usa o seu próprio certificado – escolha **Avançadas → Continuar** (os nomes dependem do navegador).
4. Escolha o idioma no topo da página.

## 3. Ler os endereços do comando

1. Ligue o DualSense ao computador com o cabo USB.
2. Na secção **Bluetooth** do painel, transfira o leitor de endereços:
   - **Windows:** `ps5-wake-reader.cmd` – faça duplo clique. Se o Windows avisar sobre um editor desconhecido,
     escolha **Mais informações → Executar mesmo assim**.
   - **macOS:** execute `pip3 install hidapi` e depois `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (sem pacotes adicionais).
3. O leitor mostra os endereços Bluetooth da consola e do comando e copia um **código de configuração**
   (`PS5WAKE:…`) para a área de transferência. Cole-o no campo **Código de configuração do leitor** – os dois
   endereços são preenchidos.

Os endereços só mudam se voltar a emparelhar o comando com a consola.

## 4. Concluir a configuração

1. Escolha a rede Wi-Fi com **Procurar**, introduza a palavra-passe e escolha o país.
2. Introduza uma **nova palavra-passe para a rede do painel** (pelo menos 8 caracteres). É obrigatória, porque a
   palavra-passe predefinida é pública.
3. Clique em **Guardar e reiniciar**. O painel fecha, o LED pisca a **azul** durante a ligação e fica **verde** quando
   está pronto.

## 5. Configurar o Wake-on-LAN

- Na aplicação de Wake-on-LAN (ou na página WoL do router), use o **endereço MAC do ESP32** mostrado no painel, a
  porta UDP **9** e o **endereço de difusão** da rede (por exemplo, `192.168.1.255`).
- Reserve um IP fixo para o ESP32 no router.
- Se a aplicação ou o router já usam o **MAC de rede da própria PS5**, pode introduzi-lo no painel – o ESP32 também
  reagirá a ele. Em redes mesh (por exemplo, ASUS AiMesh) estes pacotes podem não chegar ao ESP32; nesse caso, envie
  o WoL para o MAC do ESP32.

## Utilização diária

- **Estado da consola:** o ponto no ícone de energia no topo do painel fica verde com a PS5 ligada, laranja em modo de repouso e vermelho quando está desligada ou inacessível (cinzento: ainda não detetada). O modo de repouso só é detetado se a consola se mantiver ligada à internet.
- **Acordar a PS5:** envie um pacote Wake-on-LAN ou prima brevemente **BOOT**. O LED pisca uma vez a branco, pisca a
  ciano durante a tentativa e depois pisca **3× a verde** (a consola respondeu) ou **3× a vermelho** (sem resposta).
- Entre tentativas passam pelo menos 30 segundos; um pedido ignorado dá um piscar laranja.

## O botão BOOT

| Tempo premido | LED enquanto premido | Resultado |
|---|---|---|
| menos de 1 s | – | acordar a PS5 |
| 1–5 s | branco | nada (cancelar) |
| 5–20 s, depois soltar | vermelho (pisca a partir de 15 s) | abrir o painel de configuração |
| 20 s | roxo, a piscar depressa | apagar todas as definições e reiniciar |

Manter BOOT premido ao ligar a alimentação inicia um modo de segurança sem Wi-Fi nem Bluetooth (LED amarelo).

## Cores do LED

| Cor | Comportamento | Significado |
|---|---|---|
| roxo | fixo | não configurado – painel de configuração ativo |
| azul | pisca devagar | a ligar ao Wi-Fi |
| laranja | pisca devagar | Wi-Fi configurado, mas sem ligação há mais de 30 s |
| verde (ténue) | fixo | pronto (pode ser desligado no painel) |
| vermelho | fixo | painel de configuração ativo |
| amarelo | fixo | modo de segurança |
| branco | um piscar | Wake-on-LAN recebido ou BOOT premido |
| ciano | pisca depressa | tentativa de despertar em curso |
| verde | três piscares | a consola respondeu |
| vermelho | três piscares | a consola não respondeu |
| laranja | um piscar | pedido ignorado (tentativa em curso ou pausa de 30 s) |

## O painel de configuração

- O painel e a sua rede desligam-se após guardar, após **Fechar sem alterações** ou após 10 minutos sem atividade.
  **Prolongar (+10 min)** dá mais tempo.
- **Desligar o LED por completo**: Sem cores, piscares nem flashes. Só enquanto mantém BOOT premido o LED continua a mostrar o tempo de pressão.
- **Nome do dispositivo (opcional)**: Por ex. Sala. Usado no endereço do painel (`sala.local`), no Home Assistant e no título da página – útil com vários dispositivos.
- **Acesso a partir da rede doméstica**: Ativado por predefinição. A página fica sempre disponível na rede doméstica em `<IP do ESP32>` e `ps5-wake-xxxx.local` – inicie sessão com a palavra-passe do painel (nome de utilizador admin, se não o alterou). A rede Wi-Fi do painel não é ligada; mantenha BOOT premido 5 s para a abrir.
- **Ligação encriptada (HTTPS)**: Recomendado. O ESP32 cria o seu próprio certificado, por isso na primeira visita o navegador avisa que a ligação não é privada – compare a impressão digital abaixo e continue. Sem HTTPS, a palavra-passe e as definições circulam sem encriptação na rede. O certificado é renovado automaticamente antes de expirar (o ESP32 obtém a hora da internet); **Novo certificado** cria um novo de imediato.
- **Acordar a PS5 agora** testa o despertar a partir do painel.

## Resolução de problemas

- **LED laranja a piscar:** palavra-passe Wi-Fi errada, rede só de 5 GHz ou sinal fraco. Abra o painel, verifique o
  Wi-Fi e aproxime o ESP32 do router. O ESP32 reinicia sozinho se ficar 2 minutos sem ligação.
- **Nada acontece após enviar o Wake-on-LAN (sem piscar branco):** o pacote não chega ao ESP32. Use o MAC do ESP32, o
  endereço de difusão e a porta 9; confirme que o ESP32 não está numa rede de convidados.
- **Piscares vermelhos após uma tentativa:** verifique os endereços Bluetooth (leia-os de novo se voltou a emparelhar
  o comando) e aproxime o ESP32 da consola.
- **O leitor não encontra o comando:** use um cabo de dados (não apenas de carregamento) e ligue o comando
  diretamente ao computador.
- **O navegador avisa sobre o certificado:** é normal – o painel é encriptado com o certificado próprio do ESP32. Continue para a página; o painel mostra a impressão digital do certificado.
