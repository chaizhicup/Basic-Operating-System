/* 
    File: simple_keyboard.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2017/02/17

    Simple control of the keyboard.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* (none) */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "interrupts.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

SimpleKeyboard::SimpleKeyboard() {
    key_pressed = false;
}

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S i m p l e K e y b o a r d */
/*--------------------------------------------------------------------------*/

void SimpleKeyboard::handle_interrupt(REGS *_r) {
/* What to do when keyboard interrupt occurs? In this case, we update
   "key_pressed". */
 
    key_pressed = true;

    unsigned char status = inportb(STATUS_PORT);
    /* lowest bit of status willb e set if buffer is not empty. */
    if (status & 0x01) {
      char kc = inportb(DATA_PORT);
      if (kc >= 0) {
	key_pressed = true;
        key_code = kc;
      }    
    }
}

void SimpleKeyboard::wait() {
/* Loop until the user presses a key. */

  kb.key_pressed = false;
        
  while(kb.key_pressed == false);

}

char SimpleKeyboard::read() {
  /* Loop until the user presses a key, and then return the keycode. */
  kb.key_pressed = false;

  while (kb.key_pressed == false); 

  return kb.key_code;
}

SimpleKeyboard SimpleKeyboard::kb;

void SimpleKeyboard::init() {
  InterruptHandler::register_handler(1, &kb);
}


