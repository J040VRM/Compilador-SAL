# Compilador SAL

Este projeto implementa a primeira fase de um compilador para a linguagem SAL (Simple Academic Language), em C. Nesta etapa, o programa realiza:

- análise léxica do código-fonte;
- análise sintática por descida recursiva;
- construção da tabela de símbolos com controle de escopo;
- geração opcional de arquivos de log para tokens, tabela de símbolos e rastreamento da análise.

O compilador lê um arquivo fonte `.sal`, valida sua estrutura de acordo com a especificação da linguagem e interrompe a execução quando encontra erro léxico ou sintático, informando a linha e a coluna do problema.

## Estrutura principal

Os módulos do projeto estão organizados da seguinte forma:

- `src/`: arquivos fonte do compilador;
- `include/`: cabeçalhos públicos dos módulos;
- `examples/`: exemplos de programas SAL para teste.

Os módulos principais do compilador são:

- `main`: coordena a execução do compilador;
- `lex`: analisador léxico;
- `parser`: analisador sintático descendente recursivo;
- `symtab`: tabela de símbolos com escopos;
- `diag`: mensagens de erro e rastreamento;
- `opts`: processamento dos parâmetros de linha de comando;
- `log`: geração de arquivos auxiliares;
- `token`: definição e utilidades dos tokens.

## Como compilar

No diretório do projeto, execute:

```sh
make
```

O comando gera o executável:

```sh
./salc
```

Para remover arquivos gerados na compilação:

```sh
make clean
```

## Como executar

Uso básico:

```sh
./salc <arquivo.sal>
```

Exemplo:

```sh
./salc examples/teste.sal
```

## Opções disponíveis

O compilador aceita as seguintes opções:

```sh
./salc <arquivo.sal> [--tokens] [--symtab] [--trace]
```

### `--tokens`

Gera um arquivo com extensão `.tk`, contendo a lista de tokens reconhecidos pelo analisador léxico.

Exemplo:

```sh
./salc examples/teste.sal --tokens
```

### `--symtab`

Gera um arquivo com extensão `.ts`, contendo a tabela de símbolos consolidada por escopo.

Exemplo:

```sh
./salc examples/teste.sal --symtab
```

### `--trace`

Gera um arquivo com extensão `.trc`, contendo o rastreamento da análise sintática.

Exemplo:

```sh
./salc examples/teste.sal --trace
```

As opções podem ser usadas em conjunto:

```sh
./salc examples/teste.sal --tokens --symtab --trace
```

## Arquivos gerados

Dependendo das opções utilizadas, o compilador pode gerar:

- `programa.tk`: lista de tokens;
- `programa.ts`: tabela de símbolos;
- `programa.trc`: rastreamento da análise.

Esses arquivos são gerados na mesma pasta do arquivo `.sal` usado na execução. Por exemplo, ao executar:

```sh
./salc examples/teste_aceito.sal --tokens --symtab --trace
```

os arquivos criados serão:

```sh
examples/teste_aceito.tk
examples/teste_aceito.ts
examples/teste_aceito.trc
```

## Observação

O projeto foi desenvolvido para a fase inicial do compilador, com foco em análise léxica, análise sintática e tabela de símbolos. A geração de código para máquina virtual não faz parte desta etapa.
