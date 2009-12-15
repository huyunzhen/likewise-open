/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ADUserAddDlg : WizardDialog
{
    #region Class Data
    public UserInfo userInfo;
    #endregion
    
    #region Constructors
    public ADUserAddDlg()
    {
        InitializeComponent();
        
    }
    
    public ADUserAddDlg(IPlugInContainer container, 
                        StandardPage parentPage, 
                        string text, 
                        ADUCDirectoryNode dirnode, 
                        bool bAcountDisable, 
                        bool bNeverExpiresPwd, 
                        bool bMustChangePwd,
                        bool bUserCannotChange,
                        string copyfrom)
    : this()
    {
        this.IPlugInContainer = container;
        
        this.userInfo = new UserInfo();
        
        if (dirnode != null)
        {
            int ret = -1;
            List<LdapEntry> ldapEntries = null;
            
            ret = dirnode.LdapContext.ListChildEntriesSynchronous
            (dirnode.DistinguishedName,
            LdapAPI.LDAPSCOPE.BASE,
            "(objectClass=*)",
            null,
            false,
            out ldapEntries);
            
            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                return;
            }
            
            LdapEntry ldapNextEntry = ldapEntries[0];
            //As of now we are not getting canonicalName attribute in the list because of paging issue
            //LdapValue[] attr = ldapNextEntry.GetAttributeValues("canonicalName", dirnode.LdapContext);
            //if (attr != null && attr.Length > 0)
            //    this.lblCreatein.Text = "Create in: " + attr[0].stringData;
            
            //As of now we are taking "DistinguishedName" and spliting and displaying it.
            string[] sData = dirnode.DistinguishedName.Split(',');
            string sOutput = "";
            for (int i = sData.Length - 1; i >= 0; i--)
            {
                if (sData[i].ToString().Trim().StartsWith("DC", StringComparison.InvariantCultureIgnoreCase))
                {
                    sOutput = sData[i].ToString().Trim().Substring(3) + "." + sOutput;
                }
                else if (sData[i].ToString().Trim().StartsWith("OU", StringComparison.InvariantCultureIgnoreCase) || sData[i].ToString().Trim().StartsWith("CN", StringComparison.InvariantCultureIgnoreCase))
                {
                    sOutput += "/" + sData[i].ToString().Trim().Substring(3);
                }
            }
            if (sOutput.EndsWith("."))
            {
                sOutput += "/";
            }
            sOutput = sOutput.Replace("./", "/");
            this.userInfo.OUPath = sOutput;
            this.userInfo.domainName = dirnode.LdapContext.DomainName;
        }
        
        this.userInfo.bAcountDisable = bAcountDisable;
        this.userInfo.bMustChangePwd = bMustChangePwd;
        this.userInfo.bNeverExpiresPwd = bNeverExpiresPwd;
        this.userInfo.bCannotChangePwd = bUserCannotChange;
        this.userInfo.copyfrom = copyfrom;
        
        this.Text = text;
        
        this.AddPage(new UserAddWelcomePage(this,dirnode));
        this.AddPage(new UserAddpasswordPage(this));
        this.AddPage(new UserAddFinishPage(this, dirnode));
    }
    #endregion
    
    #region Event Handlers
    protected override void OnClick(EventArgs e)
    {
        base.OnClick(e);
        
        if (base.DialogResult == DialogResult.OK)
        {
            
        }
    }
    #endregion
}

public class UserInfo
{
    #region Class Data
    public int MinPasswordLength = 7;
    
    public string fName = "";
    public string lName = "";
    public string initials = "";
    public string fullName = "";
    public string logonName = "";
    public string userPrelogonname = "";
    public string domainName = "";
    public string dirnode = "";
    public string passWord = "";
    public string retypedpassWord = "";
    
    public bool bMustChangePwd = true;
    public bool bCannotChangePwd = false;
    public bool bNeverExpiresPwd = true;
    public bool bAcountDisable = false;
    
    public string copyfrom = "";
    
    public bool commit = false;
    public string OUPath = "";
    #endregion
}
}

