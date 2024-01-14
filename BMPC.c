//*****************************************************************
//EDGARDO ADRIÁN FRANCO MARTÍNEZ 
//(C) Agosto 2010 Versión 1.5
//Lectura, escritura y tratamiento de imagenes BMP 
//Compilación: "gcc BMP2.c -o bmpv2"
//Ejecución: "./BMP imagen.bmp"
//Ejecución actualizada "./bmpv2 parrots.bmp convertida.bmp 200"
//Observaciones "imagen.bmp" es un BMP de 24 bits

// Archivo modificado por Mariela Curiel par leer toda la imagen en la memoria 
// y hacer la conversion de los pixeles en una funci'on. Esto facilita la programacion posterior 
// con hilos. 

//*****************************************************************

/*
  Llamado de programa de ejemplo
  ./bmpv2 -i parrots.bmp -t pibes.bmp -o 2 -h 200
  ./bmpv2 -t pibes2.bmp -i parrots.bmp -h 200 -o 2
  ./bmpv2 -o 2 -h 200 -i parrots.bmp -t pibes3.bmp
*/


//*****************************************************************
//LIBRERIAS INCLUIDAS
//*****************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h> //Libreria para incluir el tipo de dato bool


//*****************************************************************
//DEFINICION DE CONSTANTES DEL PROGRAMA
//*****************************************************************
#define 	IMAGEN_TRATADA	"tratada.bmp"		//Ruta y nombre del archivo de la imagen de salida BMP

//********************************************************************************
//DECLARACION DE ESTRUCTURAS
//********************************************************************************
//Estructura para almacenar la cabecera de la imagen BMP y un apuntador a la matriz de pixeles
typedef struct BMP
{
	char bm[2];					//(2 Bytes) BM (Tipo de archivo)
	int tamano;					//(4 Bytes) Tamaño del archivo en bytes
	int reservado;					//(4 Bytes) Reservado
	int offset;						//(4 Bytes) offset, distancia en bytes entre la img y los píxeles
	int tamanoMetadatos;			//(4 Bytes) Tamaño de Metadatos (tamaño de esta estructura = 40)
	int alto;						//(4 Bytes) Ancho (numero de píxeles horizontales)
	int ancho;					//(4 Bytes) Alto (numero de pixeles verticales)
	short int numeroPlanos;			//(2 Bytes) Numero de planos de color
	short int profundidadColor;		//(2 Bytes) Profundidad de color (debe ser 24 para nuestro caso)
	int tipoCompresion;				//(4 Bytes) Tipo de compresión (Vale 0, ya que el bmp es descomprimido)
	int tamanoEstructura;			//(4 Bytes) Tamaño de la estructura Imagen (Paleta)
	int pxmh;					//(4 Bytes) Píxeles por metro horizontal
	int pxmv;					//(4 Bytes) Píxeles por metro vertical
	int coloresUsados;				//(4 Bytes) Cantidad de colores usados 
	int coloresImportantes;			//(4 Bytes) Cantidad de colores importantes
	unsigned char ***pixel; 			//Puntero a una tabla dinamica de caracteres de 3 dimensiones para almacenar los pixeles
}BMP;

// Estructura Imagen pasada a la función de hilos
typedef struct argumentos{
  BMP *imagen;  // Puntero a la estructura de imagen BMP.
  int fila_inicio;
  int fila_final;
  int filtro;
}Imagen;

//*****************************************************************
//DECLARACIÓN DE FUNCIONES
//*****************************************************************
void abrir_imagen(BMP *imagen, char ruta[]);		//Función para abrir la imagen BMP
void crear_imagen(BMP *imagen, char ruta[]);	//Función para crear una imagen BMP
void convertir_imagen(BMP *imagen,int nhilos, int filtro); //2 sera el numero de hilos	
void *concurrencia(void *args);

/*********************************************************************************************************
//PROGRAMA PRINCIPAL 
//*********************************************************************************************************/
int main (int argc, char* argv[])
{	
  int i,j,k; 				//Variables auxiliares para loops
	BMP img;				//Estructura de tipo imágen
	char IMAGEN[45];		//Almacenará la ruta de la imagen
  char *rutaTratada, argumentos[4][50];

  
  if (argc!=9) 
	{
    printf("Por favor escriba el comando de ejecución de la siguiente forma:\n %s -i imagen.bmp -t nombre_nueva_imagen.bmp -o opciones -h número_de_hilos",argv[0]);
		exit(1);
	} 

  int parametros_ingresados = 0;
  
  for(int n = 1; n < argc; n++){
    if(strcmp(argv[n], "-i") == 0){
      printf("Argumento i: %s\n", argv[n+1]);
      strcpy(argumentos[0], argv[n+1]);
      printf("%s\n",argumentos[0]);
      parametros_ingresados++;
    }
    else if(strcmp(argv[n], "-t") == 0){
      printf("%s",argumentos[0]);
      printf("Argumento t: %s\n", argv[n+1]);
      strcpy(argumentos[1], argv[n+1]);
      printf("%s\n",argumentos[1]);
      parametros_ingresados++;
    }
    else if(strcmp(argv[n], "-o") == 0){
      printf("Argumento o: %s\n", argv[n+1]);
      strcpy(argumentos[2], argv[n+1]);
      printf("%s\n",argumentos[2]);
      parametros_ingresados++;
    }
    else if(strcmp(argv[n], "-h") == 0){
      printf("%s",argumentos[0]);     
      printf("Argumento h: %s\n", argv[n+1]);
      strcpy(argumentos[3], argv[n+1]);
      printf("%s\n",argumentos[3]);
      parametros_ingresados++;
    }
  }
	//******************************************************************	
  //Verificacion de que se lograron identificar los 4 argumentos que identifican la imagen, el nombre de la nueva imagen, la opcion de filtro y el numero de hilos. 
	//******************************************************************	
  if(parametros_ingresados !=  4){
    printf("Separar los argumentos para hacer funcionar el programa de manera correcta: -i imagen.bmp -t imagen_resultante.bmp -o (1-3) -h numhilos");
    exit(1);
  }

  int opcion = atoi(argumentos[2]);
  int nhilos = atoi(argumentos[3]);

  // Se verifica que el número de la opcion este entre 1 y 3
if(!(opcion >=  1 && opcion <=3)){
  printf("El argumento de opcion debe estar dentro del rango de número 1 y 3");
  exit(1);
}


	//Almacenar la ruta de la imágen
	strcpy(IMAGEN,argumentos[0]);
	
	//***************************************************************************************************************************
	//0 Abrir la imágen BMP de 24 bits, almacenar su cabecera en la estructura img y colocar sus pixeles en el arreglo img.pixel[][]
	//***************************************************************************************************************************	
	abrir_imagen(&img,IMAGEN);
	printf("\n*************************************************************************");
	printf("\nIMAGEN: %s",IMAGEN);
	printf("\n*************************************************************************");


	printf("\nDimensiones de la imágen:\tAlto=%d\tAncho=%d\n",img.alto,img.ancho);
	convertir_imagen(&img,atoi(argumentos[3]),atoi(argumentos[2])); 
// Argumento 3 identifica el número de hilos.
// Argumento 2 identifica la opción de filtro escogida.
		
	//***************************************************************************************************************************
	//1 Crear la imágen BMP a partir del arreglo img.pixel[][]
	//***************************************************************************************************************************	

  //Almacenar la ruta donde se va a guardar la imagen
  printf("Se almacena el argumento en al apuntador de caracteres\n");
  rutaTratada = argumentos[1];
  printf("Se crea el archivo\n");
	crear_imagen(&img,rutaTratada);
	printf("\nImágen BMP tratada en el archivo: %s\n",rutaTratada);
	
	//Terminar programa normalmente	
	exit (0);	
}

//************************************************************************
//FUNCIONES 
//************************************************************************
//*************************************************************************************************************************************************
//Función para abrir la imagen, colocarla en escala de grisis en la estructura imagen imagen (Arreglo de bytes de alto*ancho  --- 1 Byte por pixel 0-255)
//Parametros de entrada: Referencia a un BMP (Estructura BMP), Referencia a la cadena ruta char ruta[]=char *ruta
//Parametro que devuelve: Ninguno
//*************************************************************************************************************************************************
void abrir_imagen(BMP *imagen, char *ruta)
{
	FILE *archivo;	//Puntero FILE para el archivo de imágen a abrir
	int i,j,k;
  unsigned char P[3];

	
	//Abrir el archivo de imágen
	archivo = fopen( ruta, "rb+" );
	if(!archivo)
	{ 
		//Si la imágen no se encuentra en la ruta dada
		printf( "La imágen %s no se encontro\n",ruta);
		exit(1);
	}
  int u;
	//Leer la cabecera de la imagen y almacenarla en la estructura a la que apunta imagen
	u = fseek( archivo,0, SEEK_SET);
	u =fread(&imagen->bm,sizeof(char),2, archivo);
	u =fread(&imagen->tamano,sizeof(int),1, archivo);
	u =fread(&imagen->reservado,sizeof(int),1, archivo);	
	u =fread(&imagen->offset,sizeof(int),1, archivo);	
	u =fread(&imagen->tamanoMetadatos,sizeof(int),1, archivo);	
	u =fread(&imagen->alto,sizeof(int),1, archivo);	
	u =fread(&imagen->ancho,sizeof(int),1, archivo);	
	u =fread(&imagen->numeroPlanos,sizeof(short int),1, archivo);	
	u =fread(&imagen->profundidadColor,sizeof(short int),1, archivo);	
	u =fread(&imagen->tipoCompresion,sizeof(int),1, archivo);
	u =fread(&imagen->tamanoEstructura,sizeof(int),1, archivo);
	u =fread(&imagen->pxmh,sizeof(int),1, archivo);
	u =fread(&imagen->pxmv,sizeof(int),1, archivo);
	u =fread(&imagen->coloresUsados,sizeof(int),1, archivo);
	u =fread(&imagen->coloresImportantes,sizeof(int),1, archivo);	

	//Validar ciertos datos de la cabecera de la imágen	
	if (imagen->bm[0]!='B'||imagen->bm[1]!='M')	
	{
		printf ("La imagen debe ser un bitmap.\n"); 
		exit(1);
	}
	if (imagen->profundidadColor!= 24) 
	{
		printf ("La imagen debe ser de 24 bits.\n"); 
		exit(1);
	}

	//Reservar memoria para la matriz de pixels 

	imagen->pixel=malloc (imagen->alto* sizeof(char *)); 
	for( i=0; i<imagen->alto; i++){
		imagen->pixel[i]=malloc (imagen->ancho* sizeof(char*));
                                     
	}

        
        for( i=0; i<imagen->alto; i++){
            for( j=0; j<imagen->ancho; j++)
	      imagen->pixel[i][j]=malloc(3*sizeof(char));
        
	}

	//Pasar la imágen a el arreglo reservado en escala de grises
	//unsigned char R,B,G;
	    
	for (i=0;i<imagen->alto;i++)
	{
		for (j=0;j<imagen->ancho;j++){  
		  for (k=0;k<3;k++) {
                      int u =fread(&P[k],sizeof(char),1, archivo);  //Byte Blue del pixel
                      imagen->pixel[i][j][k]=(unsigned char)P[k]; 	//Formula correcta
                  }
			
		}   
	}

	
	//Cerrrar el archivo
	fclose(archivo);	
}

//****************************************************************************************************************************************************
//Función para crear una imagen BMP, a partir de la estructura imagen imagen (Arreglo de bytes de alto*ancho  --- 1 Byte por pixel 0-255)
//Parametros de entrada: Referencia a un BMP (Estructura BMP), Referencia a la cadena ruta char ruta[]=char *ruta
//Parametro que devuelve: Ninguno
//****************************************************************************************************************************************************

void convertir_imagen(BMP *imagen,int nhilos, int filtro){
  //Declaracion de variables necesarias para dividir el trabajo de los hilos
  //Argumento pasado a la función que va a hacer la concurrencia
  Imagen arg_imagen;
  arg_imagen.imagen = imagen;
  arg_imagen.filtro = filtro;
  //Creación del arreglo de hilos
  pthread_t hilos[nhilos];
  //Se revisa el caso en el que el número de hilos es mayor al de la imagen para fijar que el numero de hilos se ajuste al alto de la imagen
  if(imagen->alto < nhilos) {nhilos = imagen->alto;}
  
  //Existen dos casos, cuando la división entre el alto y el número de hilos es exacta y cuando no es exacta, para cada una, tendremos un tratamiento diferente.
  bool division_exacta;
  
  int rc, incremento, restantes = -1;
  //Se mantendra un mismo incremento para las dos divisiones
  incremento = imagen->alto/nhilos;
  if(imagen->alto%nhilos==0){
    //Se marca como division exacta
    division_exacta = true;
    //Configuración inicial del hilo para división exacta
    arg_imagen.fila_inicio = 0;
    arg_imagen.fila_final = incremento;
  }
  else{
    division_exacta = false;
    restantes = imagen->alto%nhilos;
    //Configuración de las filas para división inexacta
    arg_imagen.fila_inicio = 0;
    arg_imagen.fila_final = incremento+1;
    restantes--;
  }


  for(int n = 0;n < nhilos; n++){
    //Se mira antes si se puede pasar a division exacta porque el número de filas restantes es 0
      if(restantes == 0){
        division_exacta = true;
      }
    //Verificacion de division de filas por hilos
    printf("Numero hilo: %d\n", n);
    printf("Fila inicio: %d, Fila final: %d\n",arg_imagen.fila_inicio, arg_imagen.fila_final);
    printf("Division exacta: %s\n", division_exacta ? "true" : "false");
    printf("Restantes: %d\n\n", restantes);
    rc = pthread_create(&hilos[n],NULL,&concurrencia,(void*)&arg_imagen);
    pthread_join(hilos[n],NULL);
    if(division_exacta){
      arg_imagen.fila_inicio = arg_imagen.fila_final;
      arg_imagen.fila_final+=incremento;
    }else{
      if(restantes>0){
        arg_imagen.fila_inicio=arg_imagen.fila_final;
        arg_imagen.fila_final+=incremento+1;
        restantes--;
      }
    }
  }
}
void *concurrencia(void *args){
  int i,j,k;
  unsigned char temp;
  Imagen *imagen = (Imagen*)args;
  BMP *image = imagen->imagen;
  int fila_inicio = imagen->fila_inicio;
  int fila_final = imagen->fila_final;
  for (i=fila_inicio;i<fila_final;i++) {
	  for (j=0;j<image->ancho;j++) {  
      if (imagen->filtro == 1){
        temp = (unsigned char)((image->pixel[i][j][2]+image->pixel[i][j][1] + image->pixel[i][j][0])/3);
      }else if(imagen->filtro == 2){
         temp = (unsigned char)((image->pixel[i][j][2]*0.3)+(image->pixel[i][j][1]*0.59)+ (image->pixel[i][j][0]*0.11));
        //temp = (unsigned char)((imagen->pixel[i][j][2]*0.3+imagen->pixel[i][j][1]*0.59 + imagen->pixel[i][j][0]*0.11));
      }else if(imagen->filtro == 3){
        temp = (unsigned char)((image->pixel[i][j][2]+image->pixel[i][j][1] + image->pixel[i][j][0])/3);
        image->pixel[i][j][1] = (unsigned char)(image->pixel[i][j][1] * 0.5);
        image->pixel[i][j][2] = (unsigned char)(image->pixel[i][j][2] * 0.5);
        for (k=0;k<3;k++) if (k == 0 || k == 1)  image->pixel[i][j][k] = 0; else image->pixel[i][k][k] = temp; 

        
      }	//Formula correcta
    }    
  }
  pthread_exit(NULL);
}
 
void crear_imagen(BMP *imagen, char ruta[])
{
	FILE *archivo;	//Puntero FILE para el archivo de imágen a abrir

	int i,j,k;

	//Abrir el archivo de imágen
	archivo = fopen( ruta, "wb+" );
	if(!archivo){ 
		//Si la imágen no se encuentra en la ruta dada
		printf( "La imágen %s no se pudo crear\n",ruta);
		exit(1);
	}
	printf("Llego aqui\n");
	//Escribir la cabecera de la imagen en el archivo
	fseek( archivo,0, SEEK_SET);
	fwrite(&imagen->bm,sizeof(char),2, archivo);
	fwrite(&imagen->tamano,sizeof(int),1, archivo);	
	fwrite(&imagen->reservado,sizeof(int),1, archivo);	
	fwrite(&imagen->offset,sizeof(int),1, archivo);	
	fwrite(&imagen->tamanoMetadatos,sizeof(int),1, archivo);	
	fwrite(&imagen->alto,sizeof(int),1, archivo);	
	fwrite(&imagen->ancho,sizeof(int),1, archivo);	
	fwrite(&imagen->numeroPlanos,sizeof(short int),1, archivo);	
	fwrite(&imagen->profundidadColor,sizeof(short int),1, archivo);	
	fwrite(&imagen->tipoCompresion,sizeof(int),1, archivo);
	fwrite(&imagen->tamanoEstructura,sizeof(int),1, archivo);
	fwrite(&imagen->pxmh,sizeof(int),1, archivo);
	fwrite(&imagen->pxmv,sizeof(int),1, archivo);
	fwrite(&imagen->coloresUsados,sizeof(int),1, archivo);
	fwrite(&imagen->coloresImportantes,sizeof(int),1, archivo);	
	//Pasar la imágen del arreglo reservado en escala de grises a el archivo (Deben escribirse los valores BGR)
	for (i=0;i<imagen->alto;i++)
	{
		for (j=0;j<imagen->ancho;j++){  
      for (k=0;k<3;k++)
		    fwrite(&imagen->pixel[i][j][k],sizeof(char),1, archivo);  //Escribir el Byte Blue del pixel 
		}   
	}
	//Cerrrar el archivo
	fclose(archivo);
}

