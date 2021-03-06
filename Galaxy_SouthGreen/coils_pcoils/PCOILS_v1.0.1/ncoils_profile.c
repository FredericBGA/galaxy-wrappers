
//copied from ncoils_play_without_window_maximization_psiblast_26_04.c
//only change: full output coverage of the sequence

//ncoils_play.c is specialised on giving only the scores as output
//the maximisation routine for the best overlapping window is omitted!
//SCORES BASED ON PROFILES MADE WITH PSI-BLAST

//this is the updated version of ncoil.c with the maximisation routine

#include "ncoils_profile.h"

/* Copyright (c) 2002 Robert B. Russell 
 *  EMBL, Meyerhofstrasse 1, 69917 Heidelberg, Germany 
 *    Email: russell@embl.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */


/* Rob Russell's attempt to make a COILS program */


/* this program was modified by Markus Gruber
 * Max-Planck-Institute for Developmental Biology, Spemannstr.35, 72076 T�bingen, Germany
 * Email: markus.gruber@tuebingen.mpg.de
 */

//note that this modification of the program is only good for windows lengths that are multiples of 7!!!
//(speeding up the program)

//the following modifications of Rob Russell's code were carried out:

//1)maximisation routine (finds best register in considered window)
//  this modification was made in order to make the code comply with 
//  Lupas,A.:"Predicting Coiled Coils from Protein Sequences", Science 1991, 252:1162-1164
//  this makes the program more time consuming
//2)recycling formula for new windows that uses information already calculated in previous windows
//  this modification was made to compensate the time we lost because of 1)
//3)deals with the matrix in log-space
//  this modification was also made to compensate the time we lost because of 1)
//  the file read_matrix.c also has to be changed, since it does not allow negative matrix values
//  (it has to be recompiled as well as ncoil.c itself)
//4)calculates the weight only once (this is why windows have to be multiples of 7!)
//  is supposed to speed up the program, but has only little effect
//5)The "residues" B,J,O,U,X,and Z get weight 0 and are therefore not considered in the window
//6)The weighted case (used to be option -w) is the default case. Now the non-weighted case is optional (option -nw)

// these modifications are responsible for a speedup of 6.8 in user time (according to a measurement with the UNIX "time" command)
// compared to the old version WITH maximisation routine
// this means that the modifications approximately compensate the time loss due to the maximisation routine

//IMPORTANT (also for other ncoils* programs):
//if Xs are supposed to yield a neutral Coils score (1.0) if(1) has to replace the corresponding if-clause (as in this program)
//if Xs are supposed to yield a blank Coils score (0.0) then the original if-clause has to be restored
//NOTE: 999999.875 sometimes accidentally replaces -999999.875, but this probably has no effect (remains to be tested), the flawed version works well...

FILE *PROF;
char *profile_file;

main(int argc, char *argv[]) {

	int i,j,k,l,m,n;
	int verb;
	int window,pt;
	int which,weighted;
	int nseq;
	int t,tc;
	int seqlen;
	int mode;
	int min_seg;

	char heptfile[1000];
	char *buff;
	static char defdir[]=".";
	static char *env = defdir;

	char *seq,*title,*ident;

	float min_P;

	struct hept_pref *h;

	FILE *MAT;

	//variables for profile
	float *aa_prof;

	profile_file = (char*) malloc(50*sizeof(char));

	/* defaults */
	window = 21;
	weighted = 1; //used to be 0
	verb = 0;
	mode = 0; /* 0 = column mode, 1 = fasta, 2 = concise */
	min_P = 0.5;

	if(env==NULL) {
		fprintf(stderr,"error: environment variable COILSDIR must be set\n");
		exit(-1);
	}

	strcpy(&heptfile[0],env);
	strcpy(&heptfile[strlen(heptfile)],"/new.mat");


	for(i=1; i<argc; ++i) {
           if(argv[i][0]!='-') exit_error();
	   if(strcmp(&argv[i][1],"m")==0) {
             if((i+1)>=argc) exit_error();
             strcpy(&heptfile[0],argv[i+1]);
             i++;
	   } else if(strcmp(&argv[i][1],"prof")==0) {
             if((i+1)>=argc) exit_error();
             sscanf(argv[i+1],"%s",profile_file);
             i++;
           } else if(strcmp(&argv[i][1],"win")==0) {
             if((i+1)>=argc) exit_error();
             sscanf(argv[i+1],"%d",&window);
             i++;
	   } else if(strcmp(&argv[i][1],"c")==0) {
             mode=2;
	   } else if(strcmp(&argv[i][1],"min_seg")==0) {
             if((i+1)>=argc) exit_error();
             sscanf(argv[i+1],"%d",&min_seg);
             i++;
	   } else if(strcmp(&argv[i][1],"c")==0) {
             mode=2;
	   } else if((strcmp(&argv[i][1],"f")==0) || (strcmp(&argv[i][1],"fasta")==0)) {
	     mode=1;
	   } else if((strcmp(&argv[i][1],"min_P")==0)) {
             if((i+1)>=argc) exit_error();
             sscanf(argv[i+1],"%f",&min_P);
             i++;
	   } else if(strcmp(&argv[i][1],"help")==0) {
	     exit_error();
	     //	   } else if(strcmp(&argv[i][1],"w")==0) {
	     //	     weighted=1;
      	   } else if(strcmp(&argv[i][1],"nw")==0) {
      	     weighted=0;
	   } else if(strcmp(&argv[i][1],"V")==0 || strcmp(&argv[i][1],"v")==0) {
	     verb=1;
           } else {
	     fprintf(stderr," can't understand flag/field %s\n",argv[i]);
             exit_error();
           }
        }

	if(verb) printf("Matrix file is %s\n",heptfile);
	/* Read in matrix */
	if((MAT=fopen(heptfile,"r"))==NULL) {
		fprintf(stderr,"Error reading %s\n",heptfile);
		exit(-1);
	}
	h = read_matrix(MAT);
	if(verb) {
	   for(i=0; i<strlen(AAs); ++i) if(AAs[i]!='_') {
		pt=(int)(AAs[i]-'A');
		printf("AA %c %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n",AAs[i],
	     	   h->m[pt][0],h->m[pt][1],h->m[pt][2],h->m[pt][3],h->m[pt][4],
	     	   h->m[pt][5],h->m[pt][6]);
	   }
	   for(i=0; i<h->n; ++i) {
		printf("Window %4d %1d %f %f %f %f %f\n",
			h->f[i].win,h->f[i].w,h->f[i].m_cc,h->f[i].sd_cc,h->f[i].m_g,h->f[i].sd_g,h->f[i].sc);
	   }
	}

	/* See if there is a file for our chosen window length/weight scheme */
	which = -1;
	for(i=0; i<h->n; ++i) {
		if((h->f[i].win == window) && (h->f[i].w == weighted)) { /* match */
			if(verb) printf("Found fitting data for win %4d w %d\n",window,weighted);
			which = i;
		}
	}


	/* Read in a sequence from the standard input */
	nseq = 0;
	ident = (char*) malloc(100000*sizeof(char));
	title = (char*) malloc(100000*sizeof(char));
	buff  = (char*) malloc(100000*sizeof(char));
	t = 0;
	tc = 0;

	while(fgets(buff,99999,stdin)!=NULL) {
		/* There is a memory problem - the matrix data gets corrupted under OSF after this fgets */
		for(i=0; i<strlen(buff); ++i) if(buff[i]=='\n' || buff[i]=='\r') buff[i]='\0';
		if(buff[0]=='>') {
			if(nseq>0) {
			  //profile:
			  aa_prof=get_profile(strlen(seq));
				pred_coils(seq,ident,title,h,window,which,weighted,mode,min_P,&t,&tc,min_seg,aa_prof); 
				free(seq);
			}
                        seqlen = 0;
			i=1;
			while((buff[i]!=' ') && (buff[i]!='\0') && (buff[i]!='\n') && (buff[i]!='\r')) { ident[i-1]=buff[i]; i++; }
			ident[i-1]='\0';
			i++; j=0;
			seq = (char*)malloc(sizeof(char));
			seq[0]='\0';
			while(buff[i]!='\n' && buff[i]!='\0' && buff[i]!='\r') {
				title[j]=buff[i]; i++;
				j++;
			}
			title[j]='\0';
			nseq++;
		} else {
/*			printf("Adding |%s| to |%s| = \n",buff,seq);  */
			seq=(char*)realloc(seq,(seqlen+strlen(buff)+1)*sizeof(char));
		        strcpy(&seq[seqlen],buff); 
                        seqlen=strlen(seq);
/*			printf("       |%s|\n",seq);  */
		}
	}
	if(nseq>0) {
	  //profile:
	  aa_prof=get_profile(strlen(seq));
		pred_coils(seq,ident,title,h,window,which,weighted,mode,min_P,&t,&tc,min_seg,aa_prof); 
		free(seq);
	}
	//fprintf(stderr,"%8d sequences %8d aas %8d in coil\n",nseq,t,tc);	

	free(title); free(ident);free(profile_file); 

	exit(0);

}

float *get_profile(int seqlen)
{
  float *aa_prof;
  int counter;

  if((PROF=fopen(profile_file,"r"))==NULL) {
    fprintf(stderr,"Error reading %s\n",profile_file);
    exit(-1);
  }
  
  aa_prof = (float*) malloc(26*seqlen*sizeof(float));
  
  for(counter=0;counter<seqlen;counter++)
    {
      fscanf(PROF,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",aa_prof+26*counter,aa_prof+26*counter+1,aa_prof+26*counter+2,aa_prof+26*counter+3,aa_prof+26*counter+4,aa_prof+26*counter+5,aa_prof+26*counter+6,aa_prof+26*counter+7,aa_prof+26*counter+8,aa_prof+26*counter+9,aa_prof+26*counter+10,aa_prof+26*counter+11,aa_prof+26*counter+12,aa_prof+26*counter+13,aa_prof+26*counter+14,aa_prof+26*counter+15,aa_prof+26*counter+16,aa_prof+26*counter+17,aa_prof+26*counter+18,aa_prof+26*counter+19,aa_prof+26*counter+20,aa_prof+26*counter+21,aa_prof+26*counter+22,aa_prof+26*counter+23,aa_prof+26*counter+24,aa_prof+26*counter+25);
      //fscanf(PROF,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",aa_prof);
      //counter2=fscanf(PROF,"%f %f",aa_prof,aa_prof+1);
      /* 	    fscanf(PROF,"%f %f\n",&aa_prof[0],&aa_prof[1]); */
      /* 	    for(counter2=0;counter2<26;counter2++) */
      /* 	      aa_prof[counter2]++; */
      //printf("%d\n",counter2);
    }
  fclose(PROF);
  
  //printf("aa_prof:%f\n",*(aa_prof+6*26+24));

  return aa_prof;
}

void exit_error() {
	fprintf(stderr,"format: ncoils [options] < [sequence file]\n");
	fprintf(stderr,"       -f or -fasta        [fasta output - coils as 'x', like '-x' in seg]\n");
	fprintf(stderr,"       -c                  [concise mode - which sequences have any coils (and how many)]\n");
	fprintf(stderr,"       -min_seg <int>      [for concise mode - only report sequence if >= min coil segments]\n");
	fprintf(stderr,"       -min_P <float>  [minimum score to define coil segment; DEFAULT = 1.5]\n");
	fprintf(stderr,"       -win <int>          [window size; DEFAULT = 21]\n");
	fprintf(stderr,"       -nw                 [weight all heptad positions equally]\n");
	fprintf(stderr,"       -v                  [verbose/debug mode - print extra junk]\n");
	fprintf(stderr,"       -max_seq_len <int>  [longest sequence tolerated; DEFAULT = 100 000]\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"NCOILS, Rob Russell and Andrei Lupas, 1999\n");
	fprintf(stderr," based on Lupas, Van Dyck & Stock (1991) Science 252,1162-1164\n");
        fprintf(stderr," Copyright (C) 1999 Robert B. Russell\n");
        fprintf(stderr," NCOILS comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE\n");
        fprintf(stderr," This is free software, and you are welcome to redistribute it\n");
        fprintf(stderr," under certain conditions; see LICENSE for details\n");
	exit(-1); 
}

void pred_coils(char *seq,char *ident,char *title,struct hept_pref *h,int win, int which, int weighted,int mode, float min_P, int *t, int *tc, int min_seg, float *aa_prof) {

	int i,j,k,curr_pos;
	int len,pos,best_pos,aa_pt;
	int pt;
	int total_coil_segments;
	int are_there_coils;
	int score_counter;

	float weight_of_window[7], weight_of_residue[7][strlen(seq)], keep_complete_score[7];
	float this_score,score_for_specific_position[7],score_of_residue[7][strlen(seq)],Gg,Gcc,power;
	float t1,t2,t3,t4;
	float *score;
	float *P;
	float flexible_weighting;

	char *hept_seq;

	float highest_score;

	int myi;

	//variables for profile
	int counter;
	int ic;
	float profile;

	flexible_weighting=2.5;

	highest_score=0.0;

	len=strlen(seq);

	score    = (float*)malloc(len*sizeof(float));
	P        = (float*)malloc(len*sizeof(float));
	hept_seq =  (char*)malloc(len*sizeof(char));

	score_counter=0;

/*printf("Sequence is %s length is %d\n",seq,len); */
	for(i=0; i<len; ++i) { P[i]=0.0; score[i] = -999999.; hept_seq[i] = 'x'; }

	//calculate the first window to start with
	i=0;

	this_score = -999999.;//used to be 0.0 in non-log space

	//position for maximisation routine
	  best_pos = 0;

	  //loop of the register maximisation routine
	  //(finds best register in window)
	  for(curr_pos=0; curr_pos<7; curr_pos++)
	    {
	      score_for_specific_position[curr_pos] = 0.0; //used to be 1.0 in non-log space
		weight_of_window[curr_pos]=0.0;
                for(j=0; ((j<win) && ((i+j)<len)); ++j) {
		   aa_pt = (int)(seq[i+j]-'A');
		   if(aa_pt==55)
		     aa_pt=23;
		   if((aa_pt>=0) && (aa_pt<26)) {
			pos = (j+curr_pos)%7; /* where are we in the heptad?  pos modulus 7 */
			/*			printf("AA %c in hept %c %7.5f\n",seq[i+j],('a'+pos),h->m[aa_pt][pos]);  */
			if(weighted && (pos==0 || pos==3)) { power = flexible_weighting; }
			else { power = 1.0; }
			//if((aa_pt==1)||(aa_pt==9)||(aa_pt==14)||(aa_pt==20)||(aa_pt==23)||(aa_pt==25)) {
			//power = 0.0;
			//}			
			weight_of_residue[curr_pos][j]=power;

			weight_of_window[curr_pos]+=power;



			if(aa_pt!=1&&aa_pt!=9&&aa_pt!=14&&aa_pt!=20&&aa_pt!=23&&aa_pt!=25
			   &&h->m[aa_pt][pos]!=-999999.875) {
			  
			  //non-profile method
/* 			  score_of_residue[curr_pos][j]=power*h->m[aa_pt][pos]; */
/* 			  score_for_specific_position[curr_pos]+=power*h->m[aa_pt][pos]; */

			  //profile method			  
			  profile=0.0;
			  for(ic=0;ic<26;ic++)
			    {
			      if(ic!=1&&ic!=9&&ic!=14&&ic!=20&&ic!=23&&ic!=25)
				  profile+=(*(aa_prof+ic+26*(i+j)))*exp(h->m[ic][pos]);
			    }

			  if(profile>0)
			    profile=log(profile);

			  score_of_residue[curr_pos][j]=power*profile;
			  score_for_specific_position[curr_pos]+=power*profile;
			  //end profile method
			  

		   } else {
		     score_of_residue[curr_pos][j]=0.0;
		   }
		   
		   



		    }
                }

		if(weight_of_window[curr_pos]>0) {
		  keep_complete_score[curr_pos]=score_for_specific_position[curr_pos];
		  score_for_specific_position[curr_pos] = score_for_specific_position[curr_pos]/weight_of_window[curr_pos];
		} else {
		   score_for_specific_position[curr_pos] = 0.0;
		   for(j=0;j<7;j++)
		     keep_complete_score[j]=0.0;
	        }

		if(score_for_specific_position[curr_pos]>this_score)
		  {
		    this_score=score_for_specific_position[curr_pos];
		    best_pos=curr_pos;
		  }

	    }
	  	  
                for(j=0; ((j<win) && ((i+j)<len)); ++j) {
		    aa_pt = (int)(seq[i+j]-'A');
		    if(aa_pt==55)
		      aa_pt=23;
		    if((aa_pt>=0) && (aa_pt<26)) {
			pos = (j+best_pos)%7; //where are we in the heptad?  pos modulus 7
			if(this_score>score[i+j]) { score[i+j]=this_score; hept_seq[i+j]='a'+pos; }
		    }
		}
	  
	  /*
	            j=(win-1)/2;
		    aa_pt = (int)(seq[i+j]-'A');
		    if(aa_pt==55)
		      aa_pt=23;
		    //if((aa_pt>=0) && (aa_pt<26)) {
		    if(1){
			pos = (j+best_pos)%7; //where are we in the heptad?  pos modulus 7
			score[i+j]=this_score; hept_seq[i+j]='a'+pos;
		    }
	  */		

		//this is the recycling formula for all other windows
		for(i=1;i<(len-win+1); ++i)
		  {
		    this_score = -999999.;//used to be 0.0
		    best_pos = 0;

		    aa_pt = (int)(seq[i+win-1]-'A');
		    if(aa_pt==55)
		      aa_pt=23;

	  for(curr_pos=0; curr_pos<7; curr_pos++)
	    {
	      //this used to be the loop that we got rid of...
	      //aa_pt = (int)(seq[i+win-1]-'A');
		   if((aa_pt>=0) && (aa_pt<26)){
			pos = (win+i-1+curr_pos)%7; /* where are we in the heptad?  pos modulus 7 */
			//printf("POS:%d\t%d\n",pos,aa_pt);
			if(weighted && (pos==0 || pos==3)) { power = flexible_weighting; }
			else { power = 1.0; }
			//if((aa_pt==1)||(aa_pt==9)||(aa_pt==14)||(aa_pt==20)||(aa_pt==23)||(aa_pt==25)) {
			//power = 0.0;
			//}
/*			printf("AA %c in hept %c %7.5f\n",seq[i+j],('a'+pos),h->m[aa_pt][pos]);  */

			weight_of_residue[curr_pos][i+win-1]=power;
			weight_of_window[curr_pos]=weight_of_window[curr_pos]+power-weight_of_residue[curr_pos][i-1];
			
			//if(h->m[23][0]==999999.875)
			  //printf("%f\n",h->smallest);





			if(aa_pt!=1&&aa_pt!=9&&aa_pt!=14&&aa_pt!=20&&aa_pt!=23&&aa_pt!=25
			   &&h->m[aa_pt][pos]!=-999999.875) {
			  
			  //non-profile method
/* 			  score_of_residue[curr_pos][i+win-1]=power*h->m[aa_pt][pos]; */
/* 			  score_for_specific_position[curr_pos]=keep_complete_score[curr_pos]+power*h->m[aa_pt][pos]-score_of_residue[curr_pos][i-1]; */
/* 			  printf("profile:%f %d\n",exp(h->m[aa_pt][pos]),pos); */

			  //profile method
			  profile=0.0;
			  for(ic=0;ic<26;ic++)
			    {
			      if(ic!=1&&ic!=9&&ic!=14&&ic!=20&&ic!=23&&ic!=25)
				  profile+=(*(aa_prof+ic+26*(i+win-1)))*exp(h->m[ic][pos]);
			                             //printf("pointer:%f\tlogvalue:%f\tvalue:%f\tpos:%d\n",*(aa_prof+ic+26*(i+j)),h->m[ic][pos],exp(h->m[ic][pos]),pos);
			    }
			                             //printf("profile:%f %d\n",profile,pos);

			  if(profile>0)  //important
			    profile=log(profile);

			  score_of_residue[curr_pos][i+win-1]=power*profile;
			  score_for_specific_position[curr_pos]=keep_complete_score[curr_pos]+power*profile-score_of_residue[curr_pos][i-1];
			  //end profile method

				} else {
				  score_of_residue[curr_pos][i+win-1]=0.0;
				  score_for_specific_position[curr_pos]=keep_complete_score[curr_pos]-score_of_residue[curr_pos][i-1];
				}





		    }
		   //...until here


		if(weight_of_window[curr_pos]>0.0) {
		  keep_complete_score[curr_pos]=score_for_specific_position[curr_pos];
		  score_for_specific_position[curr_pos] = score_for_specific_position[curr_pos]/weight_of_window[curr_pos];
		} else {
		   score_for_specific_position[curr_pos] = 0.0;
		  	  for(j=0;j<7;j++)
			    keep_complete_score[j]=0.0;
	        }
		if(score_for_specific_position[curr_pos]>this_score)
		  {
		    this_score=score_for_specific_position[curr_pos];
		    best_pos=curr_pos;
		  }
	    }
	  	  
                for(j=0; ((j<win) && ((i+j)<len)); ++j) {
		    aa_pt = (int)(seq[i+j]-'A');
		    if(aa_pt==55)
		      aa_pt=23;
		    if((aa_pt>=0) && (aa_pt<26)) {
		      pos = (j+i+best_pos)%7; // where are we in the heptad?  pos modulus 7
			if(this_score>score[i+j]) { score[i+j]=this_score; hept_seq[i+j]='a'+pos; }
		    }
		}
/* 	            j=(win-1)/2; */
/* 		    aa_pt = (int)(seq[i+j]-'A'); */
/* 		    if(aa_pt==55) */
/* 		      aa_pt=23; */
/* 		    //if((aa_pt>=0) && (aa_pt<26)) { */
/* 		    if(1){ */
/* 			pos = (j+i+best_pos)%7; //where are we in the heptad?  pos modulus 7 */
/* 			score[i+j]=this_score; hept_seq[i+j]='a'+pos; */
/* 		    } */

		  }

		for(i=0; i<len; ++i)
		  //for(i=j; i<len-j; ++i) //adapted to my needs
		  {
		    score[i] = exp(score[i]);
		    //printf("%f\n",score[i]);
		  }

	if(mode==1) {
		printf(">%s %s\n",ident,title);
	}
	are_there_coils=0;
	total_coil_segments=0;

		  printf("\n%s\n",ident);

	for(i=0; i<len; ++i) {
		/* Calculate P */
		t1 = 1/(h->f[which].sd_cc);
		t2 = (score[i]-(h->f[which].m_cc))/h->f[which].sd_cc;
		t3 = fabs(t2);
		t4 = pow(t3,2);
		t4 = t3*t3;
		Gcc = t1 * exp(-0.5*t4);
/*		printf("Gcc %f %f %f %f %f\n",t1cc,t2cc,t3cc,t4cc,Gcc); */
		t1 = 1/(h->f[which].sd_g);
		t2 = (score[i]-(h->f[which].m_g))/h->f[which].sd_g;
		t3 = fabs(t2);
		t4 = pow(t3,2);
		t4 = t3*t3;
		Gg = t1 * exp(-0.5*t4);
/*		printf("Gg %f %f %f %f %f\n",t1g,t2g,t3g,t4g,Gg); */
		P[i] = Gcc/(h->f[which].sc*Gg+Gcc);
		if(P[i]>=min_P) {
                   are_there_coils=1;
                   if((i==0) || (P[i-1]<min_P)) { total_coil_segments++; }
		   (*tc)++; 
                }
		(*t)++;
		if(mode==1) {
			if(P[i]>=min_P) { printf("x"); }
			else { printf("%c",seq[i]); }
			if(((i+1)%60)==0) { printf("\n"); }
		} else if(mode==0) {
			printf("%4d %c %c %7.3f %7.3f (%7.3f %7.3f)\n",i+1,seq[i],hept_seq[i],score[i],P[i],Gcc,Gg);
		}
	}
	if(mode==1) { printf("\n"); } 
        if((mode==2) && (are_there_coils==1) && (total_coil_segments>=min_seg)) {
		if(total_coil_segments==1) {
			printf("Pred %4d coil segment  : %s %s\n",total_coil_segments,ident,title);
		} else {
			printf("Pred %4d coil segments : %s %s\n",total_coil_segments,ident,title);
		}
	}		

	free(P); free(score); free(hept_seq);
}
