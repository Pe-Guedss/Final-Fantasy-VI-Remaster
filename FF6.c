#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

const float FPS = 100;

const int SCREEN_W = 960;
const int SCREEN_H = 600;
const int BCK_W = 1280;
const int BCK_H = 720;
const int raio_corote = 32;

const float THETA = M_PI/4;
const float RAIO_HEROI = 35;
const float PASSO = 0.8;
const float taxa_atualiza_camera = 0.5;

const int probabilidede_De_Fuga = 70;
const int probabilidede_De_Erro = 90;

float alpha, altura, base;

bool teclas [] = {false, false, false, false};

int CEU_H;
float X_OPCOES, Y_OPCOES;

int tam_fonte;

char vida_heroi [20];
char vida_inimigo [20];
char pontos_heroi [20];
char msg_derrota [30];
char msg_vitoria [30];
char msg_recorde [30];

ALLEGRO_FONT *FONTE, *FONTE_STATUS;

FILE *arq;

#define BAIXO 0
#define ESQUERDA 1
#define CIMA 2
#define DIREITA 3

#define TAM_OBJETIVO_X 75
#define TAM_OBJETIVO_Y 50
#define NUM_INIMIGOS 18
#define ZONA_SEGURA 80
#define DIST_AGLOMERACAO 70

#define NAVEGACAO 0
#define BATALHA 1
#define VITORIA 2
#define DERROTA 3
#define MENU 4

#define JOGAR 5
#define SAIR 6

#define ATACANDO 0
#define ESPECIAL 1
#define FUGINDO 2

//--------------------------Status Herois----------------------------------
#define VIDA_MAX_HEROI 100
#define ATAQUE_BASE_HEROI 35
#define VELOCIDADE_ATAQUES 6
#define PONTOS_BASE 40

#define HEROI true
#define INIMIGO false

bool turno = HEROI;

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=FUNÇÕES P/ NÚMEROS ALEATÓRIOS=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

//Retorna um número aleatório entre 0 e 1 (ponto flutuante)
float randf(){
    return (float) rand() / RAND_MAX;
}

//Retorna um número ponto flutuante aleatório entre um "min" e um "max"
float randFloat(float min, float max){
    return min + randf() * (max - min);
}

//Retorna um número inteiro aleatório entre 0 e o parâmetro passado - 1 (n - 1)
int random (int n){
    return rand () % n;
}

//Retorna um número aleatório inteiro entre o mínimo e máximo passados à função
int randomInt (int min, int max){
    return min + random (max - min + 1);
}


typedef struct Ponto{
	float x, y;
}Ponto;

typedef struct Heroi{

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=DESENHO DO HERÓI=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
	Ponto centro;
	Ponto centro_antes;
	int direcao;

	ALLEGRO_COLOR cor;

	int cur_Frame;
	int frame_Width;
	int frame_Height;
	int animation_Columns;

	int animation_Row;

	ALLEGRO_BITMAP *sprite_heroi;
	ALLEGRO_BITMAP *herois_batalha;
	ALLEGRO_BITMAP *atk_heroi;
	ALLEGRO_BITMAP *atk_heroi_especial;

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=INFORMAÇÕES DE STATUS=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
	int vida;
	int vida_max;
	int ataque;
	int ataque_especial;
	int pontos;

	int acao;
	int menu;
	int executa;
	bool info;
}Heroi;

typedef struct Inimigos{
	Ponto centro;
	float raio;

	int vida;
	int ataque;
	int vida_max;

	ALLEGRO_BITMAP *pix_inimigo_batalha;
	ALLEGRO_BITMAP *pix_ataque;
	ALLEGRO_COLOR hit_box;
}Inimigos;

typedef struct Ataque{
	Ponto centro;
	float raio;
	int dano;
	bool acertou;

	bool ativo;
	int cur_Frame;
	int frame_Width;
	int frame_Height;
	int animation_Columns;

	ALLEGRO_COLOR cor;
	ALLEGRO_BITMAP *sprite_padrao;
	ALLEGRO_BITMAP *sprite_especial;
}Ataque;

typedef struct Corote{
	Ponto centro;
	float raio;
	bool ativo;

	ALLEGRO_BITMAP *pix_corote;
}Corote;

Ataque ataque_heroi;

Ataque ataque_inimigo;

void init_Globais (){
	alpha = M_PI/2 - THETA;
	altura = RAIO_HEROI * sin (alpha);
	base = RAIO_HEROI * cos (THETA);

	CEU_H = 0.4*SCREEN_H;
	X_OPCOES = 0.55*SCREEN_W;
	Y_OPCOES = 0.65*SCREEN_H;

	tam_fonte = (SCREEN_W - SCREEN_H)/7;

	FONTE = al_load_font("Recursos/fontes/fonte_ofc.ttf", tam_fonte, 1);
	FONTE_STATUS = al_load_font("Recursos/fontes/fonte_ofc.ttf", 0.5*tam_fonte, 1);

	if(FONTE == NULL || FONTE_STATUS == NULL) {
		fprintf(stderr, "font file does not exist or cannot be accessed!\n");
	}

	ataque_heroi.ativo = false;
	ataque_inimigo.ativo = false;
}

void init_Corote (Corote *c, ALLEGRO_BITMAP *pixel_corote){
	c->raio = raio_corote;
	c->centro.x = 0.32*SCREEN_W;
	c->centro.y = 0.75*SCREEN_H;
	c->ativo = true;
	c->pix_corote = pixel_corote;
}

void init_Heroi (Heroi *h, ALLEGRO_BITMAP *sprite_heroi, ALLEGRO_BITMAP *herois_batalha, 
		ALLEGRO_BITMAP *atk_heroi, ALLEGRO_BITMAP *atk_heroi_especial){
	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=DESENHO DO HERÓI=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
	h->centro.x = RAIO_HEROI;
	h->centro.y = SCREEN_H - RAIO_HEROI;
	h->cor = al_map_rgb (255, 0, 0);
	h->direcao = CIMA;

	//=*=*=*=*=*=*=*=*=*=*==**=*=*=*=*=SPRITES=*=*=*=*=*=*=*=*==*=*=*=*=*=*=*=*=*=*=*=*=
	h->cur_Frame = 0;
	h->frame_Width = 57;
	h->frame_Height = 90;
	h->animation_Columns = 3;

	h->animation_Row = CIMA;

	h->sprite_heroi = sprite_heroi;
	h->herois_batalha = herois_batalha;
	h->atk_heroi = atk_heroi;
	h->atk_heroi_especial = atk_heroi_especial;

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=INFORMAÇÕES DE STATUS=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
	h->centro_antes.x = h->centro.x;
	h->centro_antes.y = h->centro.y;

	h->vida = VIDA_MAX_HEROI;
	h->vida_max = h->vida;
	h->ataque = ATAQUE_BASE_HEROI;
	h->pontos = 0;
	h->info = false;

	h->acao = ATACANDO;
	h->menu = JOGAR;
	h->executa = 0;
}

void init_Inimigo (Inimigos *inim, ALLEGRO_BITMAP *inimigo_batalha, ALLEGRO_BITMAP *pix_ataque, int tipo){
	if (tipo == 0){
		inim->raio = 20;
		inim->vida = 50;
		inim->vida_max = inim->vida;
		inim->ataque = 15;
	}

	else if (tipo == 1){
		inim->raio = 30;
		inim->vida = 70;
		inim->vida_max = inim->vida;
		inim->ataque = 20;
	}

	else if (tipo == 2){
		inim->raio = 35;
		inim->vida = 90;
		inim->vida_max = inim->vida;
		inim->ataque = 25;
	}

	else if (tipo == 3){
		inim->raio = 40;
		inim->vida = 120;
		inim->vida_max = inim->vida;
		inim->ataque = 30;
	}

	inim->centro.x = randFloat (inim->raio, SCREEN_W - inim->raio);
	inim->centro.y = randFloat (inim->raio, SCREEN_H - inim->raio);

	inim->pix_inimigo_batalha = inimigo_batalha;
	inim->pix_ataque = pix_ataque;
	inim->hit_box = al_map_rgb (255, 255, 255);
}

float distancia_Euclideana (Ponto a, Ponto b){
	return sqrt (pow(a.x - b.x, 2) + pow (a.y - b.y, 2));
}

int colisao_Inimigos (Inimigos inimigo_atual, Inimigos inimigos_anteriores[], int n){
	for (int i = 0; i < n; i++){
		if (distancia_Euclideana (inimigo_atual.centro, inimigos_anteriores[i].centro) <
			inimigo_atual.raio + inimigos_anteriores[i].raio + DIST_AGLOMERACAO){
			return 1;
		}
	}
	return 0;
}

void desenha_Cenario_Naveg (Inimigos inimigo [], ALLEGRO_BITMAP *navigation_bck, Ponto camera_pos){

	al_draw_bitmap (navigation_bck, -camera_pos.x, camera_pos.y, 0);
	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=DESENHO DOS INIMIGOS (TEMPORÁRIO)=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
	//for (int i = 0; i < NUM_INIMIGOS; i++){
	//	al_draw_circle (inimigo[i].centro.x, inimigo[i].centro.y, inimigo[i].raio, inimigo[i].hit_box, 2);
	//}
}

void desenha_Pontos_Naveg (Heroi h, ALLEGRO_BITMAP *info_bar){

	al_draw_bitmap (info_bar, 0.01*SCREEN_W, 0.02*SCREEN_H, 0);

	sprintf(pontos_heroi, "Pontuação: %d", h.pontos);
	al_draw_text (FONTE_STATUS, al_map_rgb (0, 0, 0), 0.12*SCREEN_W, 0.025*SCREEN_H, 
					ALLEGRO_ALIGN_LEFT, pontos_heroi);

	sprintf (pontos_heroi, "Vida: %d", h.vida);
	al_draw_text (FONTE_STATUS, al_map_rgb (0, 0, 0), 0.12*SCREEN_W, 0.025*SCREEN_H + 0.4*tam_fonte, 
					ALLEGRO_ALIGN_LEFT, pontos_heroi);

	sprintf (pontos_heroi, "Ataque: %d", h.ataque);
	al_draw_text (FONTE_STATUS, al_map_rgb (0, 0, 0), 0.12*SCREEN_W, 0.025*SCREEN_H + 0.8*tam_fonte, 
					ALLEGRO_ALIGN_LEFT, pontos_heroi);
}

void desenha_Heroi_Naveg (Heroi h){

	int fx = (h.cur_Frame % h.animation_Columns) * h.frame_Width;
	int fy = h.animation_Row * h.frame_Height;

	al_draw_bitmap_region (h.sprite_heroi, fx, fy, h.frame_Width, h.frame_Height,
							h.centro.x - h.frame_Width/2, h.centro.y - h.frame_Height/2, 0);
}

void desenha_Corote (Corote c, int tempo){
	if (tempo >= 0 && tempo <= 3*(int)FPS/12){
		al_draw_bitmap (c.pix_corote, c.centro.x - 32, c.centro.y - 32, 0);
	}
	else if (tempo >= 3*(int)FPS/12 && tempo <= 6*(int)FPS/12){
		al_draw_bitmap (c.pix_corote, c.centro.x - 32, c.centro.y - 32 + 5, 0);
	}
	else if (tempo >= 6*(int)FPS/12 && tempo <= 9*(int)FPS/12){
		al_draw_bitmap (c.pix_corote, c.centro.x - 32, c.centro.y - 32, 0);
	}
	else if (tempo >= 9*(int)FPS/12 && tempo <= (int)FPS){
		al_draw_bitmap (c.pix_corote, c.centro.x - 32, c.centro.y - 32 -5, 0);
	}
}

void processa_Tecla_Naveg (Heroi *h, int tecla, ALLEGRO_SAMPLE_INSTANCE *som_tab){

    switch (tecla){
		case ALLEGRO_KEY_UP:
		{
			teclas [DIREITA] = false;
        	teclas [CIMA] = true;
       		teclas [ESQUERDA] = false;
   			teclas [BAIXO] = false;

   			h->animation_Row = CIMA;

        	break;
        }

       	case ALLEGRO_KEY_DOWN:
       	{
       		teclas [DIREITA] = false;
        	teclas [CIMA] = false;
        	teclas [ESQUERDA] = false;
        	teclas [BAIXO] = true;

        	h->animation_Row = BAIXO;

       		break;
        }

        case ALLEGRO_KEY_LEFT:
        {
        	teclas [DIREITA] = false;
        	teclas [CIMA] = false;
        	teclas [ESQUERDA] = true;
       		teclas [BAIXO] = false;			        

       		h->animation_Row = ESQUERDA;

			break;
		}

		case ALLEGRO_KEY_RIGHT:
		{
			teclas [DIREITA] = true;
			teclas [CIMA] = false;
        	teclas [ESQUERDA] = false;
        	teclas [BAIXO] = false;

        	h->animation_Row = DIREITA;

			break;
		}

		case ALLEGRO_KEY_TAB:
		{
			h->info = true;
			al_play_sample_instance (som_tab);
			break;
		}
	}
}

int chegou_Objetivo (Heroi h){
	if (h.centro.x >= SCREEN_W - TAM_OBJETIVO_X &&
		h.centro.y <= TAM_OBJETIVO_Y)
		return 1;
	else
		return 0;
}

void desenha_Placar_Vitoria (int pontos, ALLEGRO_BITMAP *tela_vitoria){
	int recorde;

	al_draw_bitmap (tela_vitoria, 0, 0, 0);

	arq = fopen("Recorde.txt", "r");

	fscanf (arq, "%d", &recorde);

	if (pontos > recorde){
		fclose (arq);

		arq = fopen("Recorde.txt", "w");
		fprintf (arq, "%d", pontos);

		sprintf (msg_vitoria, "Seu novo recorde: %d!", pontos);

		al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.5*SCREEN_W, 0.3*SCREEN_H, ALLEGRO_ALIGN_CENTRE,
						msg_vitoria);

		sprintf (msg_recorde, "Seu antigo recorde: %d", recorde);

		al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.5*SCREEN_W, 0.5*SCREEN_H, ALLEGRO_ALIGN_CENTRE,
						msg_recorde);
	}
	else {

		sprintf (msg_vitoria, "Sua Pontuação: %d", pontos);

		al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.5*SCREEN_W, 0.3*SCREEN_H, ALLEGRO_ALIGN_CENTRE,
						msg_vitoria);

		sprintf (msg_recorde, "Seu recorde: %d", recorde);

		al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.5*SCREEN_W, 0.5*SCREEN_H, ALLEGRO_ALIGN_CENTRE,
						msg_recorde);
	}

	fclose (arq);
}

int detectou_Inimigo (Heroi h, Inimigos inim []){
	for (int i = 0; i < NUM_INIMIGOS; i++){
		if (distancia_Euclideana(h.centro, inim[i].centro) - RAIO_HEROI < inim[i].raio && inim[i].vida > 0){
			return i+1;
		}
	}
	return 0;
}

void desenha_Cenario_Batalha (Heroi heroi_1, ALLEGRO_BITMAP *opts_atk, ALLEGRO_BITMAP *indicador,
								ALLEGRO_BITMAP *bck){

	al_draw_bitmap (bck, 0, 0, ALLEGRO_FLIP_HORIZONTAL);
	al_draw_bitmap (opts_atk , X_OPCOES, Y_OPCOES, 0);
	al_draw_bitmap (indicador, X_OPCOES + 36, Y_OPCOES + 36 + heroi_1.acao*40, 0);
}

void desenha_Heroi_Batalha (Heroi *heroi_1){

	if (heroi_1->vida < 1) heroi_1->vida = 0;

	//Escreve os pontos de vida restantes
	sprintf(vida_heroi, "Vida restante: %d", heroi_1->vida);

	al_draw_text (FONTE_STATUS, al_map_rgb (255, 255, 255), 0.99*SCREEN_W, 10, ALLEGRO_ALIGN_RIGHT, vida_heroi);

	//Faz a representação gráfica dos pontos de vida restantes
	al_draw_filled_rounded_rectangle (0.75*SCREEN_W, 0.4*SCREEN_H, 0.95*SCREEN_W, 0.45*SCREEN_H, 2, 1.8, 
								al_map_rgb (183, 178, 255));
	al_draw_filled_rounded_rectangle (0.95*SCREEN_W - 0.2*SCREEN_W*((float)heroi_1->vida/heroi_1->vida_max), 
									0.4*SCREEN_H, 0.95*SCREEN_W, 0.45*SCREEN_H, 2, 
									1.8, al_map_rgb (255, 0, 0));
	al_draw_rounded_rectangle(0.75*SCREEN_W, 0.4*SCREEN_H, 0.95*SCREEN_W, 0.45*SCREEN_H, 2, 1.8, 
								al_map_rgb (255, 255, 255), 4);
	//Desenha o personagem
	al_draw_rotated_bitmap(heroi_1->herois_batalha, 0, 0, 
							SCREEN_W - 1.2*al_get_bitmap_width (heroi_1->herois_batalha), 
							SCREEN_H - 1.8*al_get_bitmap_height (heroi_1->herois_batalha), -0.15, 0);
}

void desenha_Inimigo_Batalha (Inimigos *inimigo){
	if (inimigo->vida < 1) inimigo->vida = 0;

	//Escreve os pontos de vida restantes
	sprintf(vida_inimigo, "Vida restante: %d", inimigo->vida);

	al_draw_text (FONTE_STATUS, al_map_rgb (255, 255, 255), 0.01*SCREEN_W, 10, ALLEGRO_ALIGN_LEFT, vida_inimigo);

	//Faz a representação gráfica dos pontos de vida restantes
	al_draw_filled_rounded_rectangle (0.05*SCREEN_W, 0.48*SCREEN_H, 0.25*SCREEN_W, 0.53*SCREEN_H, 2, 1.8, 
			al_map_rgb (183, 178, 255));
	al_draw_filled_rounded_rectangle (0.05*SCREEN_W, 0.48*SCREEN_H, 
			0.05*SCREEN_W + ((float)inimigo->vida/inimigo->vida_max)*0.2*SCREEN_W, 0.53*SCREEN_H, 
			2, 1.8, al_map_rgb (255, 0, 0));
	al_draw_rounded_rectangle(0.05*SCREEN_W, 0.48*SCREEN_H, 0.25*SCREEN_W, 0.53*SCREEN_H, 2, 1.8, 
			al_map_rgb (255, 255, 255), 4);
 	
	//Desenha o personagem
	al_draw_bitmap (inimigo->pix_inimigo_batalha, 0.02*SCREEN_W, 0.48*SCREEN_H, 0);
}

void processa_Tecla_Batalha (Heroi *h, int tecla, ALLEGRO_SAMPLE_INSTANCE *som_selecionar){
	switch (tecla){
        case ALLEGRO_KEY_UP:{
       		h->acao--;

       		//Controla o indicador para que não aponte para opções inexistentes
       		if (h->acao < ATACANDO) h->acao = FUGINDO;
       		else if (h->acao > FUGINDO) h->acao = ATACANDO;

       		printf ("\nA acao agora eh: %d", h->acao);
       		al_play_sample_instance (som_selecionar);
        	break;
        }

        case ALLEGRO_KEY_DOWN:{
	        h->acao++;

	        //Controla o indicador para que não aponte para opções inexistentes
	        if (h->acao < ATACANDO) h->acao = FUGINDO;
       		else if (h->acao > FUGINDO) h->acao = ATACANDO;

       		printf ("\nA acao agora eh: %d", h->acao);
       		al_play_sample_instance (som_selecionar);
	        break;
		}

        case ALLEGRO_KEY_ENTER:{
	        h->executa = 1;
	        printf ("\nA acao executada eh: %d", h->acao);
	        al_play_sample_instance (som_selecionar);
	        break;
   	 	}
    }
}

void Init_Ataque_Heroi (Heroi *h, int modo){
	ataque_heroi.centro.x = SCREEN_W - al_get_bitmap_width (h->herois_batalha);
	ataque_heroi.centro.y = SCREEN_H - 3.2*al_get_bitmap_height (h->herois_batalha);
	ataque_heroi.raio = h->ataque;

	if (randomInt (0, 100) < probabilidede_De_Erro){
		ataque_heroi.acertou = true;
	}
	else{
		ataque_heroi.acertou = false;
	}

	if (modo == ATACANDO)
		ataque_heroi.dano = h->ataque + randomInt (0, 10);
	else if (modo == ESPECIAL)
		ataque_heroi.dano = h->ataque_especial;

	ataque_heroi.ativo = true;

	ataque_heroi.sprite_padrao = h->atk_heroi;
	ataque_heroi.sprite_especial = h->atk_heroi_especial;
}

void Init_Ataque_Inimigo (Inimigos *i){
	ataque_inimigo.centro.x = 0.07*SCREEN_W;
	ataque_inimigo.centro.y = 0.52*SCREEN_H;
	ataque_inimigo.dano = i->ataque;

	ataque_inimigo.ativo = true;

	ataque_inimigo.cur_Frame = 0;
	ataque_inimigo.frame_Width = 100;
	ataque_inimigo.frame_Height = 100;
	ataque_inimigo.animation_Columns = 4;

	ataque_inimigo.sprite_padrao = i->pix_ataque;
	ataque_inimigo.cor = al_map_rgb (randomInt(0, 255), randomInt(0, 255), randomInt(0, 255));
}

void desenha_Ataque_Heroi (int tipo){
	if (tipo == ATACANDO){
		al_draw_rotated_bitmap(ataque_heroi.sprite_padrao, 0, 0, 
			ataque_heroi.centro.x, ataque_heroi.centro.y, -0.15, 0);
	}
	else if (tipo == ESPECIAL){
		al_draw_rotated_bitmap(ataque_heroi.sprite_especial, 0, 0, 
			ataque_heroi.centro.x, ataque_heroi.centro.y, -0.15, 0);
	}
}

void desenha_Ataque_Inimigo (){

	int fx = (ataque_inimigo.cur_Frame % ataque_inimigo.animation_Columns) * ataque_inimigo.frame_Width;
	int fy = 0;

	al_draw_bitmap_region (ataque_inimigo.sprite_padrao, fx, fy, ataque_inimigo.frame_Width, 
			ataque_inimigo.frame_Height, ataque_inimigo.centro.x - ataque_inimigo.frame_Width/2, 
			ataque_inimigo.centro.y - ataque_inimigo.frame_Height/2, 0);
}

void atualiza_Ataque_Heroi (Inimigos *i, int *ciclos, int *controle_tempo){

	if (ataque_heroi.ativo){
		ataque_heroi.centro.x -= VELOCIDADE_ATAQUES;
		ataque_heroi.centro.y = -0.15*ataque_heroi.centro.x + (SCREEN_H - 3.2*al_get_bitmap_height (ataque_heroi.sprite_padrao));

		if (ataque_heroi.centro.x <= 0.05*SCREEN_W){
			ataque_heroi.ativo = false;
			turno = INIMIGO;
			*ciclos = 0;
			*controle_tempo = 1;
			if (ataque_heroi.acertou){
				i->vida -= ataque_heroi.dano;
			}
			else{
				return;
			}
		}
	}
}

void atualiza_Ataque_Inimigo (int *ciclos, Heroi *h, int *controle_tempo){

	if (ataque_inimigo.ativo){
		ataque_inimigo.centro.x += VELOCIDADE_ATAQUES;
		ataque_inimigo.centro.y = 0.15*ataque_inimigo.centro.y + 0.52*SCREEN_H;

		if (*ciclos >= 0 && *ciclos <= 3*(int)FPS/12)
			ataque_inimigo.cur_Frame = 0;

		else if (*ciclos >= 3*(int)FPS/12 && *ciclos <= 6*(int)FPS/12)
			ataque_inimigo.cur_Frame = 1;

		else if (*ciclos >= 6*(int)FPS/12 && *ciclos <= 9*(int)FPS/12)
			ataque_inimigo.cur_Frame = 2;

		else if (*ciclos >= 9*(int)FPS/12 && *ciclos <= (int)FPS)
			ataque_inimigo.cur_Frame = 3;

		if (ataque_inimigo.centro.x >= SCREEN_W - 5*RAIO_HEROI){
			ataque_inimigo.ativo = false;
			h->vida -= ataque_inimigo.dano;
			turno = HEROI;
			*ciclos = 0;
			*controle_tempo = 1;
		}
	}
}

int processa_Acao_Heroi (Heroi *h, Inimigos *i){

	if (h->executa == 1){

		h->executa = 0;

		if (h->acao == ATACANDO){
			Init_Ataque_Heroi (h, h->acao);
			return BATALHA;
		}

		else if (h->acao == ESPECIAL){
			h->ataque_especial = randomInt (15, 50);
			Init_Ataque_Heroi (h, h->acao);
			return BATALHA;
		}

		else if (h->acao == FUGINDO){

			if (randomInt (0, 100) < probabilidede_De_Fuga){
				h->centro.x = h->centro_antes.x;
				h->centro.y = h->centro_antes.y;

				for (int cont = 0; cont < 4; cont++){
					teclas [cont] = false;
				}

				h->acao = ATACANDO;

				ataque_heroi.ativo = false;

				h->cur_Frame = 0;
				return NAVEGACAO;
			}
			else{
				turno = INIMIGO;

				for (int cont = 0; cont < 4; cont++){
					teclas [cont] = false;
				}

				return BATALHA;
			}
		}
	}

	return BATALHA;
}

void desenha_Placar_Derrota (int pontos, ALLEGRO_BITMAP *tela_derrota){
	int recorde;

	al_draw_bitmap (tela_derrota, 0, 0, 0);

	arq = fopen("Recorde.txt", "r");
	fscanf (arq, "%d", &recorde);
	sprintf (msg_vitoria, "Sua Pontuação atual: %d", pontos);
	al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.5*SCREEN_W, 0.3*SCREEN_H, ALLEGRO_ALIGN_CENTRE,
						msg_vitoria);

	sprintf (msg_recorde, "Seu recorde: %d", recorde);
	al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.5*SCREEN_W, 0.5*SCREEN_H, ALLEGRO_ALIGN_CENTRE,
						msg_recorde);
	fclose (arq);
}

void desenha_Menu (int posicao, ALLEGRO_BITMAP *menu_inicial, ALLEGRO_BITMAP *jogar, 
					ALLEGRO_BITMAP *sair, ALLEGRO_BITMAP *indicador, int ciclos){
	int recorde;

	al_draw_bitmap (menu_inicial, 0, 0, 0);

	if (posicao == JOGAR){
		if (ciclos <= 0.5*(int)FPS){
			al_draw_bitmap (jogar, 0.08*SCREEN_W, 0.45*SCREEN_H, 0);
			al_draw_bitmap (sair, 0.08*SCREEN_W, 0.65*SCREEN_H, 0);
		}

		else{
			al_draw_bitmap (sair, 0.08*SCREEN_W, 0.65*SCREEN_H, 0);
		}
	}

	if (posicao == SAIR){
		if (ciclos <= 0.5*(int)FPS){
			al_draw_bitmap (sair, 0.08*SCREEN_W, 0.65*SCREEN_H, 0);
			al_draw_bitmap (jogar, 0.08*SCREEN_W, 0.45*SCREEN_H, 0);
		}

		else{
			al_draw_bitmap (jogar, 0.08*SCREEN_W, 0.45*SCREEN_H, 0);
		}
	}

	al_draw_bitmap (indicador, 0.03*SCREEN_W, 0.53*SCREEN_H + (posicao-JOGAR) * 0.2 * SCREEN_H + tam_fonte/2, 0);

	arq = fopen("Recorde.txt", "r");
	fscanf (arq, "%d", &recorde);
	sprintf (msg_vitoria, "Recorde atual: %d", recorde);
	al_draw_text (FONTE, al_map_rgb (255, 255, 255), 0.95*SCREEN_W, 0.9*SCREEN_H, ALLEGRO_ALIGN_RIGHT,
						msg_vitoria);

	fclose (arq);
}

void processa_Tecla_Menu (Heroi *h, int tecla, int *ciclos){
	switch (tecla){
		case ALLEGRO_KEY_UP:{
			h->menu--;
			//Controla o indicador para que não aponte para opções inexistentes
			if (h->menu < JOGAR) h->menu = SAIR;
			else if (h->menu > SAIR) h->menu = JOGAR;

			printf ("\nA acao agora eh: %d", h->menu);
	        break;
	        }

		case ALLEGRO_KEY_DOWN:{
			h->menu++;

			//Controla o indicador para que não aponte para opções inexistentes
			if (h->menu < JOGAR) h->menu = SAIR;
			else if (h->menu > SAIR) h->menu = JOGAR;

			printf ("\nA acao agora eh: %d", h->menu);
			break;
		}

		case ALLEGRO_KEY_ENTER:{
			h->executa = 1;
			printf ("\nA acao executada eh: %d", h->menu);
			break;
		}
	
	}
}

int processa_Acao_Menu (Heroi *h){
	if (h->executa == 1){

		h->executa = 0;

		if (h->menu == JOGAR){
			printf ("\n\nJOGAR!\n\n");
			return JOGAR;
		}

		else if (h->menu == SAIR){
			return SAIR;
		}
	}

	return MENU;
}

int processa_Tecla_Placares (int tecla, int modo_de_jogo){
	switch (tecla){

		case ALLEGRO_KEY_ENTER:{
			for (int i = 0; i < 4; i++)
				teclas [i] = false;

			return MENU;
			break;
			}

		case ALLEGRO_KEY_ESCAPE:{
			return SAIR;
			break;
		}
	}
	return modo_de_jogo;
}

void troca_sprite_heroi (Heroi *h, int ciclos){
	if (ciclos >= 0 && ciclos <= 3*(int)FPS/12)
		h->cur_Frame = 1;

	else if (ciclos >= 3*(int)FPS/12 && ciclos <= 6*(int)FPS/12)
		h->cur_Frame = 0;

	else if (ciclos >= 6*(int)FPS/12 && ciclos <= 9*(int)FPS/12)
		h->cur_Frame = 2;

	else if (ciclos >= 9*(int)FPS/12 && ciclos <= (int)FPS)
		h->cur_Frame = 0;
}

int ciclos = 0;

int main(int argc, char **argv){
	
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER *timer = NULL;

	ALLEGRO_BITMAP *sprite_heroi;
	ALLEGRO_BITMAP *atk_heroi;
	ALLEGRO_BITMAP *info_bar;
	ALLEGRO_BITMAP *navigation_bck;
	ALLEGRO_BITMAP *pixel_corote;
	ALLEGRO_BITMAP *opts_atk;
	ALLEGRO_BITMAP *menu_inicial;
	ALLEGRO_BITMAP *jogar;
	ALLEGRO_BITMAP *sair;
	ALLEGRO_BITMAP *indicador;
	ALLEGRO_BITMAP *indicador_batalha;
	ALLEGRO_BITMAP *tela_derrota;
	ALLEGRO_BITMAP *tela_vitoria;
	ALLEGRO_BITMAP *batalha_bck;
	ALLEGRO_BITMAP *herois_batalha;
	ALLEGRO_BITMAP *inimigo_dificil;
	ALLEGRO_BITMAP *atk_inimigo_dificil;
	ALLEGRO_BITMAP *atk_heroi_especial;
	ALLEGRO_BITMAP *inimigo_medio;
	ALLEGRO_BITMAP *atk_inimigo_medio;
	ALLEGRO_BITMAP *inimigo_facil;
	ALLEGRO_BITMAP *atk_inimigo_facil;
	ALLEGRO_BITMAP *inimigo_insano;
	ALLEGRO_BITMAP *atk_inimigo_insano;

	ALLEGRO_SAMPLE *musica_menu = NULL;
	ALLEGRO_SAMPLE *selecionar = NULL;
	ALLEGRO_SAMPLE *som_corote = NULL;
	ALLEGRO_SAMPLE *som_jogar = NULL;
	ALLEGRO_SAMPLE *som_sair = NULL;
	ALLEGRO_SAMPLE *som_info = NULL;
	ALLEGRO_SAMPLE *musica_naveg = NULL;
	ALLEGRO_SAMPLE *musica_batalha = NULL;

	ALLEGRO_SAMPLE_INSTANCE *instancia_menu = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_selecionar = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_som_corote = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_som_jogar = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_som_sair = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_som_info = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_musica_naveg = NULL;
	ALLEGRO_SAMPLE_INSTANCE *instancia_musica_batalha = NULL;

	//Número do inimigo encontrado na exploração
	int n = 0;
	int controle_tempo = 0;

	srand (time(NULL));
   
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=ROTINAS DE INICIALIZAÇÃO=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
    
	//inicializa o Allegro
	if(!al_init()) {
		fprintf(stderr, "failed to initialize allegro!\n");
		return -1;
	}
	
    //inicializa o módulo de primitivas do Allegro
    if(!al_init_primitives_addon()){
		fprintf(stderr, "failed to initialize primitives!\n");
        return -1;
    }	
	
	//inicializa o modulo que permite carregar imagens no jogo
	if(!al_init_image_addon()){
		fprintf(stderr, "failed to initialize image module!\n");
		return -1;
	}
   
	//cria um temporizador que incrementa uma unidade a cada 1.0/FPS segundos
    timer = al_create_timer(1.0 / FPS);
    if(!timer) {
		fprintf(stderr, "failed to create timer!\n");
		return -1;
	}
 
	//cria uma tela com dimensoes de SCREEN_W, SCREEN_H pixels
	display = al_create_display(SCREEN_W, SCREEN_H);
	if(!display) {
		fprintf(stderr, "failed to create display!\n");
		al_destroy_timer(timer);
		return -1;
	}

	//instala o teclado
	if(!al_install_keyboard()) {
		fprintf(stderr, "failed to install keyboard!\n");
		return -1;
	}
	
	//inicializa o modulo allegro que carrega as fontes
	al_init_font_addon();

	//inicializa o modulo allegro que entende arquivos tff de fontes
	if(!al_init_ttf_addon()) {
		fprintf(stderr, "failed to load tff font module!\n");
		return -1;
	}

	arq = fopen("Recorde.txt", "r");
	if (!arq){
		fclose (arq);
		arq = fopen("Recorde.txt", "w");
		fprintf (arq, "%d", 0);
	}
	fclose (arq);
	
 	//cria a fila de eventos
	event_queue = al_create_event_queue();
	if(!event_queue) {
		fprintf(stderr, "failed to create event_queue!\n");
		al_destroy_display(display);
		return -1;
	}

	if(!al_install_audio()){
		fprintf (stderr, "failed to install the audio!\n");
		return -1;
	}

	if(!al_init_acodec_addon()){
		fprintf (stderr, "failed to initialize the acodec!\n");
		return -1;
	}

	//Artes com fundo removível
	sprite_heroi = al_load_bitmap ("Recursos/Sprites/herois/sprite_heroi.png");
	al_convert_mask_to_alpha (sprite_heroi, al_map_rgb (197, 63, 114));
	pixel_corote = al_load_bitmap ("Recursos/Sprites/diversos/Dorot.png");
	al_convert_mask_to_alpha (pixel_corote, al_map_rgb (197, 63, 114));
	atk_heroi = al_load_bitmap ("Recursos/Sprites/ataques/atk_heroi.png");
	al_convert_mask_to_alpha (atk_heroi, al_map_rgb (197, 63, 114));
	jogar = al_load_bitmap ("Recursos/Sprites/diversos/JOGAR.png");
	al_convert_mask_to_alpha (jogar, al_map_rgb (181, 80, 136));
	sair = al_load_bitmap ("Recursos/Sprites/diversos/SAIR.png");
	al_convert_mask_to_alpha (sair, al_map_rgb (181, 80, 136));
	indicador = al_load_bitmap ("Recursos/Sprites/diversos/INDICADOR.png");
	al_convert_mask_to_alpha (indicador, al_map_rgb (181, 80, 136));
	indicador_batalha = al_load_bitmap ("Recursos/Sprites/diversos/indicador_batalha.png");
	al_convert_mask_to_alpha (indicador_batalha, al_map_rgb (181, 80, 136));
	herois_batalha = al_load_bitmap ("Recursos/Sprites/herois/herois_batalha.png");
	inimigo_dificil = al_load_bitmap ("Recursos/Sprites/inimigos/inimigo_dificil.png");
	atk_inimigo_dificil = al_load_bitmap ("Recursos/Sprites/ataques/atk_inim_dificil.png");
	atk_heroi_especial = al_load_bitmap ("Recursos/Sprites/ataques/atk_heroi_especial.png");
	inimigo_medio = al_load_bitmap ("Recursos/Sprites/inimigos/inimigo_medio.png");
	atk_inimigo_medio = al_load_bitmap ("Recursos/Sprites/ataques/atk_inimigo_medio.png");
	inimigo_facil = al_load_bitmap ("Recursos/Sprites/inimigos/inimigo_facil.png");
	atk_inimigo_facil = al_load_bitmap ("Recursos/Sprites/ataques/atk_inim_facil.png");
	inimigo_insano = al_load_bitmap ("Recursos/Sprites/inimigos/inimigo_insano.png");
	atk_inimigo_insano = al_load_bitmap ("Recursos/Sprites/ataques/atk_inimigo_insano.png");

	//Artes completas
	info_bar = al_load_bitmap ("Recursos/Interfaces/Info_bar.png");
	navigation_bck = al_load_bitmap ("Recursos/Fundos/background.png");
	opts_atk = al_load_bitmap ("Recursos/Interfaces/opts_atk.png");
	menu_inicial = al_load_bitmap ("Recursos/Interfaces/Menu_inicial.png");
	tela_derrota = al_load_bitmap ("Recursos/Interfaces/tela_derrota.png");
	tela_vitoria = al_load_bitmap ("Recursos/Interfaces/tela_vitoria.png");
	batalha_bck = al_load_bitmap ("Recursos/Fundos/background_batalha.png");

	al_reserve_samples (8);

	musica_menu = al_load_sample ("Recursos/Sons/musica_menu.ogg");
	selecionar = al_load_sample ("Recursos/Sons/selecionar.ogg");
	som_corote = al_load_sample ("Recursos/Sons/corote.ogg");
	som_jogar = al_load_sample ("Recursos/Sons/jogar.ogg");
	som_sair = al_load_sample ("Recursos/Sons/sair.ogg");
	som_info = al_load_sample ("Recursos/Sons/info.ogg");
	musica_naveg = al_load_sample ("Recursos/Sons/musica_naveg.ogg");
	musica_batalha = al_load_sample ("Recursos/Sons/musica_batalha.ogg");

	instancia_menu = al_create_sample_instance (musica_menu);
	al_set_sample_instance_gain (instancia_menu, 0.45);
	al_set_sample_instance_playmode (instancia_menu, ALLEGRO_PLAYMODE_LOOP);
	al_attach_sample_instance_to_mixer (instancia_menu, al_get_default_mixer());

	instancia_selecionar = al_create_sample_instance (selecionar);
	al_set_sample_instance_gain (instancia_selecionar, 0.3);
	al_attach_sample_instance_to_mixer (instancia_selecionar, al_get_default_mixer());

	instancia_som_corote = al_create_sample_instance (som_corote);
	al_set_sample_instance_gain (instancia_som_corote, 0.4);
	al_attach_sample_instance_to_mixer (instancia_som_corote, al_get_default_mixer());

	instancia_som_jogar = al_create_sample_instance (som_jogar);
	al_set_sample_instance_gain (instancia_som_jogar, 0.4);
	al_attach_sample_instance_to_mixer (instancia_som_jogar, al_get_default_mixer());

	instancia_som_sair = al_create_sample_instance (som_sair);
	al_set_sample_instance_gain (instancia_som_jogar, 0.4);
	al_attach_sample_instance_to_mixer (instancia_som_sair, al_get_default_mixer());

	instancia_som_info = al_create_sample_instance (som_info);
	al_set_sample_instance_gain (instancia_som_info, 0.3);
	al_attach_sample_instance_to_mixer (instancia_som_info, al_get_default_mixer());

	instancia_musica_naveg = al_create_sample_instance (musica_naveg);
	al_set_sample_instance_gain (instancia_musica_naveg, 0.45);
	al_set_sample_instance_playmode (instancia_musica_naveg, ALLEGRO_PLAYMODE_LOOP);
	al_attach_sample_instance_to_mixer (instancia_musica_naveg, al_get_default_mixer());

	instancia_musica_batalha = al_create_sample_instance (musica_batalha);
	al_set_sample_instance_gain (instancia_musica_batalha, 0.45);
	al_set_sample_instance_playmode (instancia_musica_batalha, ALLEGRO_PLAYMODE_LOOP);
	al_attach_sample_instance_to_mixer (instancia_musica_batalha, al_get_default_mixer());


	//registra na fila os eventos de tela (ex: clicar no X na janela)
	al_register_event_source(event_queue, al_get_display_event_source(display));
	//registra na fila os eventos de tempo: quando o tempo altera de t para t+1
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	//registra na fila os eventos de teclado (ex: pressionar uma tecla)
	al_register_event_source(event_queue, al_get_keyboard_event_source());

//=*=*=*=*=*=*=*=*=*=*=*=*=INICIAR OS PERSONAGENS E PARÂMETROS=*=*=*=*=*=*=*=*=*=*=*=*=
	init_Globais ();

	Ponto camera_pos;
	camera_pos.x = 0;
	camera_pos.y = -(BCK_H-SCREEN_H);

	Heroi heroi_1;
	heroi_1.menu = JOGAR;
	heroi_1.executa = 0;

	Corote Dorot;

	Inimigos inimigo [NUM_INIMIGOS];

	//inicia o temporizador
	al_start_timer(timer);
	
	int playing = 1;
	int modo_de_jogo = MENU;

//=*=*=*=*=*=*=*=*=*=*=*=*=JOGO RODANDO=*=*=*=*=*=*=*=*=*=*=*=*=
	while(playing) {
		ALLEGRO_EVENT ev;
		//espera por um evento e o armazena na variavel de evento ev
		al_wait_for_event(event_queue, &ev);

		//se o tipo de evento for um evento do temporizador, ou seja, se o tempo passou de t para t+1
		if(ev.type == ALLEGRO_EVENT_TIMER) {

			if (ciclos > FPS){
				ciclos = 0;
				printf ("\nCiclos: %d", ciclos);
				if (controle_tempo > 0)
					controle_tempo--;
			}

			if (modo_de_jogo == MENU){
				al_play_sample_instance (instancia_menu);
				desenha_Menu (heroi_1.menu, menu_inicial, jogar, sair, indicador, ciclos);
				modo_de_jogo = processa_Acao_Menu (&heroi_1);
			}
			//Inicializa o mapa e os parâmetros do jogo
			else if (modo_de_jogo == JOGAR){

				al_stop_sample_instance(instancia_menu);
				al_play_sample_instance (instancia_som_jogar);
				al_rest (1.2);
				init_Heroi (&heroi_1, sprite_heroi, herois_batalha, atk_heroi, atk_heroi_especial);

				//Faz a renderização dos inimigos no mapa
				for (int i = 0; i < NUM_INIMIGOS; i++){
					int tipo = random(4);
					if (tipo == 0){
						init_Inimigo (&inimigo [i], inimigo_facil, atk_inimigo_facil, tipo);
					}
					if (tipo == 1){
						init_Inimigo (&inimigo [i], inimigo_medio, atk_inimigo_medio, tipo);
					}
					else if (tipo == 2){
						init_Inimigo (&inimigo [i], inimigo_dificil, atk_inimigo_dificil, tipo);
					}
					else if (tipo == 3){
						init_Inimigo (&inimigo [i], inimigo_insano, atk_inimigo_insano, tipo);
					}

					if (colisao_Inimigos (inimigo [i], inimigo, i) || 
					(inimigo [i].centro.x - inimigo [i].raio < ZONA_SEGURA + DIST_AGLOMERACAO &&
						inimigo [i].centro.y + inimigo[i].raio > SCREEN_H - ZONA_SEGURA - DIST_AGLOMERACAO) ||
						(inimigo [i].centro.x + inimigo [i].raio > SCREEN_W - TAM_OBJETIVO_X &&
						inimigo [i].centro.y - inimigo[i].raio < TAM_OBJETIVO_Y))
						i--;
				}

				//Garante que é possível pegar o corote
				do{
					init_Corote (&Dorot, pixel_corote);
				}while ((Dorot.centro.x < ZONA_SEGURA  + DIST_AGLOMERACAO && 
						Dorot.centro.y > SCREEN_H - ZONA_SEGURA - DIST_AGLOMERACAO) ||
						(Dorot.centro.x > SCREEN_W - TAM_OBJETIVO_X - DIST_AGLOMERACAO && 
							Dorot.centro.y < TAM_OBJETIVO_Y + DIST_AGLOMERACAO));

				modo_de_jogo = NAVEGACAO;
			}

			else if (modo_de_jogo == SAIR){
				playing = 0;
				al_stop_sample_instance(instancia_menu);
				al_play_sample_instance (instancia_som_sair);
				al_rest (0.4);
			}

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=NAVEGAÇÃO=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
			else if (modo_de_jogo == NAVEGACAO){
				al_stop_sample_instance (instancia_musica_batalha);
				al_play_sample_instance (instancia_musica_naveg);
				desenha_Cenario_Naveg (inimigo, navigation_bck, camera_pos);

				desenha_Heroi_Naveg (heroi_1);

				if (Dorot.ativo)
					desenha_Corote (Dorot, ciclos);

				if (heroi_1.info)
					desenha_Pontos_Naveg (heroi_1, info_bar);

				if (detectou_Inimigo (heroi_1, inimigo)){
					n = detectou_Inimigo (heroi_1, inimigo);
					printf ("\n\nINIMIGO %d ENCONTRADO!\n\n", n);
					turno = HEROI;
					modo_de_jogo = BATALHA;
				}

				if ((distancia_Euclideana (heroi_1.centro, Dorot.centro) < Dorot.raio + 10) && 
						Dorot.ativo)
				{
					Dorot.ativo = false;
					heroi_1.vida += 100;
					if (heroi_1.vida > heroi_1.vida_max)
						heroi_1.vida_max = heroi_1.vida;
					heroi_1.ataque += 5;
					al_play_sample_instance (instancia_som_corote);
				}

				if (chegou_Objetivo (heroi_1)){
					modo_de_jogo = VITORIA;
				}
			}

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=BATALHA=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
			else if (modo_de_jogo == BATALHA){
				al_stop_sample_instance (instancia_musica_naveg);
				al_play_sample_instance (instancia_musica_batalha);

				desenha_Cenario_Batalha (heroi_1, opts_atk, indicador_batalha, batalha_bck);
				desenha_Heroi_Batalha (&heroi_1);
				desenha_Inimigo_Batalha (&inimigo [n-1]);

				al_flip_display ();

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=TURNO DE ATAQUE DO HERÓI=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
				if (!ataque_inimigo.ativo && controle_tempo == 0){

					modo_de_jogo = processa_Acao_Heroi (&heroi_1, &inimigo [n-1]);

					if (ataque_heroi.ativo){
						desenha_Ataque_Heroi (heroi_1.acao);
						atualiza_Ataque_Heroi (&inimigo [n - 1], &ciclos, &controle_tempo);
						al_flip_display ();
					}
				}

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=VERIFICA SE O HEROI ESTÁ MORTO=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
				if (heroi_1.vida == 0 && !ataque_inimigo.ativo){
					modo_de_jogo = DERROTA;
				}

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=VERIFICA SE O INIMIGO ESTÁ MORTO=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
				if (inimigo[n-1].vida < 1 && !ataque_heroi.ativo){
					modo_de_jogo = NAVEGACAO;
					turno = HEROI;
					ataque_heroi.ativo = false;
					ataque_inimigo.ativo = false;
					for (int cont = 0; cont < 4; cont++){
						teclas [cont] = false;
					}
					heroi_1.pontos += PONTOS_BASE + 1.5*inimigo[n-1].ataque;
					heroi_1.cur_Frame = 0;	
				}

	//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=TURNO DE ATAQUE DO INIMIGO=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
				if (turno == INIMIGO && !ataque_heroi.ativo && controle_tempo == 0){

					if (!ataque_inimigo.ativo){
						Init_Ataque_Inimigo (&inimigo [n-1]);
						ciclos = 0;
					}

					desenha_Ataque_Inimigo ();
					atualiza_Ataque_Inimigo (&ciclos, &heroi_1, &controle_tempo);
					al_flip_display ();
				}
			}

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=PLACAR DE FIM DE JOGO (VITÓRIA)=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
			else if (modo_de_jogo == VITORIA){
				al_stop_sample_instance (instancia_musica_batalha);
				al_stop_sample_instance (instancia_musica_naveg);
				al_stop_sample_instance (instancia_menu);

				desenha_Placar_Vitoria (heroi_1.pontos, tela_vitoria);
				al_flip_display ();
			}

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=PLACAR DE FIM DE JOGO (DERROTA)=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
			else if (modo_de_jogo == DERROTA){
				al_stop_sample_instance (instancia_musica_batalha);
				al_stop_sample_instance (instancia_musica_naveg);
				al_stop_sample_instance (instancia_menu);

				desenha_Placar_Derrota (heroi_1.pontos, tela_derrota);
				al_flip_display ();
			}

			//atualiza a tela (quando houver algo para mostrar)
			al_flip_display();
			
			if(al_get_timer_count(timer)%(int)FPS == 0)
				printf("\n%d segundos se passaram...", (int)(al_get_timer_count(timer)/FPS));
		}
		//se o tipo de evento for o fechamento da tela (clique no x da janela)
		else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			playing = 0;
		}

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=CONTROLES DE NAVEGAÇÃO E BATALHA=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
		//se o tipo de evento for um pressionar de uma tecla
		else if(ev.type == ALLEGRO_EVENT_KEY_DOWN && !ataque_heroi.ativo && !ataque_inimigo.ativo) {
			//imprime qual tecla foi
			printf("\ncodigo tecla: %d", ev.keyboard.keycode);

			if (modo_de_jogo == NAVEGACAO){
				ciclos = 0;
				processa_Tecla_Naveg (&heroi_1, ev.keyboard.keycode, instancia_som_info);
			}

			else if (modo_de_jogo == BATALHA && controle_tempo == 0){
				processa_Tecla_Batalha (&heroi_1, ev.keyboard.keycode, instancia_selecionar);
			}

			else if (modo_de_jogo == MENU){
				processa_Tecla_Menu (&heroi_1, ev.keyboard.keycode, &ciclos);
				al_play_sample_instance (instancia_selecionar);
			}

			else if (modo_de_jogo == VITORIA || modo_de_jogo == DERROTA){
				modo_de_jogo = processa_Tecla_Placares (ev.keyboard.keycode, modo_de_jogo);
			}
		}

		//se o tipo de evento for um soltar de uma tecla
		else if(ev.type == ALLEGRO_EVENT_KEY_UP && !ataque_heroi.ativo && !ataque_inimigo.ativo) {
			printf("\ncodigo tecla: %d", ev.keyboard.keycode);

			if (modo_de_jogo == NAVEGACAO){

				switch (ev.keyboard.keycode){
					case ALLEGRO_KEY_UP:
						teclas [CIMA] = false;
						heroi_1.cur_Frame = 0;
	        		break;

	       			case ALLEGRO_KEY_DOWN:
	        			teclas [BAIXO] = false;
	        			heroi_1.cur_Frame = 0;
	        		break;

	        		case ALLEGRO_KEY_LEFT:
	        			teclas [ESQUERDA] = false;
	        			heroi_1.cur_Frame = 0;
	        		break;

	       			case ALLEGRO_KEY_RIGHT:
	        			teclas [DIREITA] = false;
	        			heroi_1.cur_Frame = 0;
	        		break;

	        		case ALLEGRO_KEY_TAB:
	        			heroi_1.info = false;
	        		break;
				}
			}
		}

//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=MOVIMENTAÇÃO DO HERÓI=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
		if (modo_de_jogo == NAVEGACAO && !ataque_heroi.ativo){

			if (heroi_1.centro.y - 35 - PASSO >= 0 && teclas [CIMA]){

				heroi_1.centro_antes.x = heroi_1.centro.x;
				heroi_1.centro_antes.y = heroi_1.centro.y + PASSO;

	        	heroi_1.centro.y -= (teclas[CIMA] * PASSO);
	        	
	        	troca_sprite_heroi (&heroi_1, ciclos);

	        	if (heroi_1.centro.y - RAIO_HEROI < SCREEN_H/2 && camera_pos.y < 0){
	        		camera_pos.y += taxa_atualiza_camera*(BCK_H/SCREEN_H);
	        		Dorot.centro.y += taxa_atualiza_camera*(BCK_W/SCREEN_W);
	        	}
			}

	        else if (heroi_1.centro.y + 35 + PASSO <= SCREEN_H && teclas [BAIXO]){

	        	heroi_1.centro_antes.x = heroi_1.centro.x;
				heroi_1.centro_antes.y = heroi_1.centro.y - PASSO;

	        	heroi_1.centro.y += (teclas [BAIXO] * PASSO);

	        	troca_sprite_heroi (&heroi_1, ciclos);

	        	if (heroi_1.centro.y + RAIO_HEROI > SCREEN_H/2 && camera_pos.y > -(BCK_H-SCREEN_H)){
	        		camera_pos.y -= taxa_atualiza_camera*(BCK_H/SCREEN_H);
	        		Dorot.centro.y -= taxa_atualiza_camera*(BCK_W/SCREEN_W);
	        	}
	        }

	        else if (heroi_1.centro.x - 14 - PASSO >= 0 && teclas [ESQUERDA]){

	        	heroi_1.centro_antes.x = heroi_1.centro.x + PASSO;
				heroi_1.centro_antes.y = heroi_1.centro.y;

	        	heroi_1.centro.x -= (teclas [ESQUERDA] * PASSO);

	        	troca_sprite_heroi (&heroi_1, ciclos);

	        	if (heroi_1.centro.x - RAIO_HEROI < SCREEN_W/2 && camera_pos.x > 0){
	        		camera_pos.x -= taxa_atualiza_camera*(BCK_W/SCREEN_W);
	        		Dorot.centro.x += taxa_atualiza_camera*(BCK_W/SCREEN_W);
	        	}
	        }

	        else if (heroi_1.centro.x + 14 + PASSO <= SCREEN_W && teclas [DIREITA]){

	        	heroi_1.centro_antes.x = heroi_1.centro.x - PASSO;
				heroi_1.centro_antes.y = heroi_1.centro.y;

	        	heroi_1.centro.x += (teclas [DIREITA] * PASSO);

	        	troca_sprite_heroi (&heroi_1, ciclos);

	        	if (heroi_1.centro.x + RAIO_HEROI > SCREEN_W/2 && camera_pos.x + SCREEN_W < BCK_W){
	        		camera_pos.x += taxa_atualiza_camera*(BCK_W/SCREEN_W);
	        		Dorot.centro.x -= taxa_atualiza_camera*(BCK_W/SCREEN_W);
	        	}
	        }

	        else{
	        	heroi_1.cur_Frame = 0;
	        }
    	}
    	ciclos++;
	} //fim do while
     
//=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=procedimentos de fim de jogo=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
							//(fecha a tela, limpa a memoria, etc)
	al_destroy_timer(timer);
	al_destroy_display(display);
	al_destroy_event_queue(event_queue);

	al_destroy_sample (musica_menu);
	al_destroy_sample (selecionar);
	al_destroy_sample (som_corote);
	al_destroy_sample (som_jogar);
	al_destroy_sample (som_sair);
	al_destroy_sample (som_info);
	al_destroy_sample (musica_naveg);
	al_destroy_sample (musica_batalha);

	al_destroy_sample_instance (instancia_menu);
	al_destroy_sample_instance (instancia_selecionar);
	al_destroy_sample_instance (instancia_som_corote);
	al_destroy_sample_instance (instancia_som_jogar);
	al_destroy_sample_instance (instancia_som_sair);
	al_destroy_sample_instance (instancia_som_info);
	al_destroy_sample_instance (instancia_musica_naveg);
	al_destroy_sample_instance (instancia_musica_batalha);

	al_destroy_bitmap (sprite_heroi);
	al_destroy_bitmap (atk_heroi);
	al_destroy_bitmap (info_bar);
	al_destroy_bitmap (navigation_bck);
	al_destroy_bitmap (pixel_corote);
	al_destroy_bitmap (opts_atk);
	al_destroy_bitmap (menu_inicial);
	al_destroy_bitmap (jogar);
	al_destroy_bitmap (sair);
	al_destroy_bitmap (indicador);
	al_destroy_bitmap (indicador_batalha);
	al_destroy_bitmap (tela_derrota);
	al_destroy_bitmap (tela_vitoria);
	al_destroy_bitmap (batalha_bck);
	al_destroy_bitmap (herois_batalha);
	al_destroy_bitmap (inimigo_dificil);
	al_destroy_bitmap (atk_inimigo_dificil);
	al_destroy_bitmap (atk_heroi_especial);
	al_destroy_bitmap (inimigo_medio);
	al_destroy_bitmap (atk_inimigo_medio);
	al_destroy_bitmap (inimigo_facil);
	al_destroy_bitmap (atk_inimigo_facil);
	al_destroy_bitmap (inimigo_insano);
	al_destroy_bitmap (inimigo_insano);
   
	return 0;
}