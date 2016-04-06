#include "uthreads.h"
#include <stdio.h>
#include <unistd.h>
#include "thread_classes.h"
#include <iostream>
#include "general_macros.h"
using namespace std;


void f()
{
			printf("starting f. My id is %d\n",uthread_get_tid());

	
	for(long a = 1;a > 0; a++)
	{
		if(a%102415342==0)
		{
			printf("Running f. My id is \n",uthread_get_tid());


		}
		
		if(a%202414542==0)
		{
			printf("F:Killing g %ld\n",a);
			printf("Got return code %d\n",uthread_terminate(2));
			
		}
		
		/*
		if(a%10245153423==0)
		{
			printf("f: Blocking h\n");
			uthread_block(3);
		}
		
		if(a%10248153423==0)
		{
			printf("f: Blocking i\n");
			uthread_block(4);
		}
		if(a%10231533423==0)
		{
			printf("f: resuming f\n");
			uthread_resume(1);
		}
		*/

	}
	
}


void g()
{
	printf("Entered g\n");
	
	for(long a = 1;a > 0; a++)
	{
		if(a%10249342==0)
		{
			printf("Running g %ld\n",a);

		}
		
		/*
		if(a%102433423==0)
		{
			printf("g: Blocking g\n");
			uthread_block(2);
		}
		
		if(a%1015334234==0)
		{
			printf("g:Resuming f\n");
			uthread_resume(1);
		}
		*/

	}
	
	
}

void h()
{
	printf("Entered h\n");
	
	for(long a = 1;a > 0; a++)
	{
		if(a%102413342==0)
		{
			printf("h:Running h %ld\n",a);

		}
		if(a%802414542==0)
		{
			printf("H:Killing f %ld\n",a);
			printf("Got return code %d\n",uthread_terminate(1));
			
		}
		
		/*
		if(a%1024143234==0)
		{
			printf("h:blocking g\n");
			uthread_block(2);
		}
		
		if(a%4024143234==0)
		{
			printf("h:resuming f\n");
			uthread_resume(1);
		}
		*/

	}
	
	
}

void i()
{
	//int id = uthread_get_tid();
	//printf("Entered i. According to function, has id %d. Number of quantums run is %d\n",id,uthread_get_quantums(id));
	
	for(long a = 1;a > 0; a++)
	{
		if(a%102415342==0)
		{
			printf("Running i %ld\n",a);

		}
		
		if(a%1094145342==0)
		{
			printf("i:Killing myself %ld\n",a);
			printf("I:Got return code %d\n",uthread_terminate(uthread_get_tid()));
			
		}
		/*
		if(a%13153242==0)
		{
				printf("Entered i. According to function, has id %d. Number of quantums run is %d\n",uthread_get_tid(),uthread_get_quantums(uthread_get_tid()));

			printf(": Putting myself to sleep for 1\n");
			uthread_sleep(1);
		}
		*/
		
	}
	
	
}

void j()
{
	int id = uthread_get_tid();
	printf("Entered j. According to function, has id %d. Number of quantums run is %d\n",id,uthread_get_quantums(id));
	
	for(long a = 1;a > 0; a++)
	{
		if(a%102415342==0)
		{
			printf("Running j %ld\n",a);

		}
		
		if(a%1025324342==0)
		{
			printf("Killing main %ld\n",a);
			printf("Got return code %d\n",uthread_terminate(0));
			
		}
		/*
		if(a%1315342==0)
		{
				printf("Entered j. According to function, has id %d. Number of quantums run is %d\n",uthread_get_tid(),uthread_get_quantums(uthread_get_tid()));

			printf("J: Putting myself to sleep for 3\n");
			uthread_sleep(3);
		}
		*/
	
		
		

	}
	
	
}


void k()
{
	int id = uthread_get_tid();
	//printf("Entered k. According to function, has id %d. Number of quantums run is %d\n",id,uthread_get_quantums(4));
	
	for(long a = 1;a > 0; a++)
	{
		if(a%102415342==0)
		{
			printf("Running k %ld\n",a);

		}
		if(a%1024145342==0)
		{
			printf("F:Killing non-existant %ld\n",a);
			printf("Got return code %d\n",uthread_terminate(9));
			
		}
		/*
		if(a%1315342==0)
		{
			
				printf("Entered k. According to function, has id %d. Number of quantums run is %d\n",uthread_get_tid(),uthread_get_quantums(uthread_get_tid()));

			printf("K: Putting myself to sleep for 4\n");
			uthread_sleep(4);
		}
		*/
	}
	
	
}


int main(void)
{
	
	uthread_init(4000);
	printf("Came out of init, and Quantum call is %d\n",uthread_get_total_quantums());
	
	uthread_spawn(f);
	uthread_spawn(g);
	uthread_spawn(h);
	uthread_spawn(i);
	
	
	

	

	
	for(long a = 1;a > 0; a++)
	{
		
		if(a%102415342==0)
		{
			printf("Running main %ld\n",a);


		}
		
		if(a%202415342==0)
		{
			printf("Spawning j %ld\n",a);
			uthread_spawn(j);
		}
		
		if(a%402415342==0)
		{
			printf("Spawning k %ld\n",a);
			uthread_spawn(k);
		}
		/*
		if(a%102415342==0)
		{
			printf("Running main %ld, has id %d. has run for %d quantums\n",a,uthread_get_tid(),uthread_get_quantums(0));


		}
		
		if(a%13102415342==0)
		{
			printf("Main: Putting myself to sleep for 4\n");
			uthread_sleep(4);
		}
	
		
		
		if(a%102414323==0)
		{
			printf("Main: Blocking f\n");
			uthread_block(1);
		}
		
		
		if(a%202414323==0)
		{
			printf("Main: Blocking main\n");
			uthread_block(0);
		}

		
		if(a%102343234==0)
		{
			printf("Main: Resuming h\n");
			uthread_resume(3);
		}
		
		if(a%502343234==0)
		{
			printf("Main: Resuming g\n");
			uthread_resume(2);
		}
		*/

	}

}
