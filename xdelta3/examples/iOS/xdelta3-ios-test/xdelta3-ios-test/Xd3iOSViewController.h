//
//  Xd3iOSViewController.h
//  xdelta3-ios-test
//
//  Created by Joshua MacDonald on 6/16/12.
//  Copyright (c) 2011, 2012 Joshua MacDonald. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface Xd3iOSViewController : UIViewController <UITextViewDelegate> {
    NSString *inputSeed;
}
- (IBAction)startTest:(id)sender;
@property (weak, nonatomic) IBOutlet UITextField *theSeed;
@property (weak, nonatomic) IBOutlet UITextView *theView;
@property (atomic, retain) NSMutableString *theOutput;
@property (nonatomic) BOOL inTest;

@end
