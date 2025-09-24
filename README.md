# Projeto IoT – Monitoramento de Motos em Pátio

📌 Nota: Projeto desenvolvido para fins acadêmicos na disciplina de Advanced Business Development with .NET

Este projeto implementa um protótipo IoT para monitoramento de motos em pátios organizados por fileiras.

O sistema identifica quando uma moto entra ou sai de uma fileira, atualiza o inventário em tempo real e envia os dados via MQTT para um aplicativo front-end.

A ideia é mostrar como uma solução IoT pode ser aplicada em cenários de logística e gestão de veículos.

## Indice

- [Arquitetura](#arquitetura)
- [Componentes (Simulação)](#componentes-simulação-wokwi)
- [Fluxo do Sistema](#fluxo-do-sistema)
- [MQTT - Estrutura de Tópicos](#mqtt--estrutura-de-tópicos)
- [Como Testar](#como-testar)
- [Bibliotecas Usadas](#bibliotecas-usadas)

## Arquitetura
Versão real (implantação prática)

- Sensores de barreira IR industriais → detectam a passagem das motos em entradas/saídas.
- Leitores UHF RFID → identificam cada moto através de etiquetas passivas.
- ESP32 → atua como nó IoT, conecta os sensores e publica dados via MQTT.
- Aplicativo Mobile (Expo/React Native) → assina os tópicos MQTT e exibe o estado de cada fileira.
- Atuadores (ex.: LED ou semáforo) → indicam status de ocupação local.

Versão simulada (Wokwi)

- Botões (Pushbuttons) → simulam os sensores IR de passagem.
- IDs mockados (A1, A2, …) → simulam as tags RFID reais.
- LED + resistor → atuador que acende quando há pelo menos uma moto na fileira.
- ESP32 DevKit v1 → envia mensagens via MQTT para um broker público (Mosquitto).
- Aplicativo → recebe e exibe o inventário e eventos em tempo real.

## Componentes (simulação Wokwi)
- ESP32
- 2 Pushbuttons (entrada/saída)
- 1 LED (vermelho)
- 1 Resistor 220 Ω

Conexões
- Botão LEFT → 32 ↔ GND
- Botão RIGHT → 22 ↔ GND
- LED → Ânodo no 15 / Cátodo no resistor 220 Ω → GND

## Fluxo do Sistema
```
flowchart LR

    A[Moto passa] --> B[Sensor IR (simulado com botão)]

    B --> C[Leitura de ID (mockado)]

    C --> D[ESP32]

    D --> E[Broker MQTT]

    E --> F[Aplicativo Mobile (Expo)]

    D --> G[LED indica presença na fileira]
```

## MQTT – Estrutura de Tópicos
- Eventos (não retidos)

```yard/rows/{rowId}/events```
```
{
  "type": "ENTRY",
  "side": "LEFT",
  "motoId": "A1",
  "ts": "2025-09-23T22:00:00Z"
}
```
- Inventário (retido)

```yard/rows/{rowId}/inventory```
```
{
  "rowId": "A",
  "present": ["A1","A2"],
  "ts": "2025-09-23T22:00:05Z"
}
```
- Status do nó (LWT)

```yard/nodes/{nodeId}/status``` → ```online``` / ```offline```

## Como testar
Abrir o projeto no Wokwi.

Iniciar a simulação.

Pressionar botão LEFT (32) → gera evento ENTRY e acende LED.

Pressionar botão RIGHT (22) → gera evento EXIT e apaga LED se não restar nenhuma moto.

Assinar os tópicos MQTT:
mosquitto_sub -h test.mosquitto.org -t "yard/rows/A/#" -v

## Bibliotecas usadas
PubSubClient – MQTT (Nick O’Leary)

ArduinoJson – formatação de mensagens (Benoit Blanchon)